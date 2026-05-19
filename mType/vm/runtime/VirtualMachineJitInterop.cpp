#include "VirtualMachine.hpp"
#include <cstddef>
#include "../../errors/RuntimeException.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../environment/registry/ClassDefinition.hpp"
#include "../../value/ValueObject.hpp"
#include "../../value/ValueShim.hpp"
#include "../../value/ObjectInstancePool.hpp"
#include "../bytecode/BytecodeProgram.hpp"

namespace vm::runtime
{
    value::Value VirtualMachine::callFunctionFromJit(const std::string& funcName,
                                                      std::span<const value::Value> args)
    {
        auto funcMeta = program->getFunction(funcName);
        if (!funcMeta)
        {
            throw std::runtime_error("JIT interpreter fallback: function not found: " + funcName);
        }
        return callFunctionFromJitDirect(funcName, funcMeta, args);
    }

    value::Value VirtualMachine::callFunctionFromJitDirect(const std::string& funcName,
                                                            const bytecode::BytecodeProgram::FunctionMetadata* funcMeta,
                                                            std::span<const value::Value> args)
    {
        size_t savedIP = instructionPointer;
        size_t savedCallStackDepth = callStack.size();
        size_t savedStackSize = stackManager->size();

        for (const auto& arg : args)
        {
            stackManager->push(arg);
        }
        for (size_t i = args.size(); i < funcMeta->localCount; ++i)
        {
            stackManager->push(std::monostate{});
        }

        CallFrame frame;
        frame.returnAddress = savedIP;
        frame.frameBase = savedStackSize;
        frame.localBase = savedStackSize;
        frame.functionName = program->internFrameName(funcName);
        frame.thisInstance = nullptr;
        pushCallFrame(std::move(frame));

        instructionPointer = funcMeta->startOffset;

        // Set the flag so any CALL inside the fallback mini-interpret routes
        // through handleCall (interpreter) instead of re-spawning a nested
        // JIT dispatch + callFunctionFromJit cycle. RAII restore even on
        // exception so finite recursion through this path stays correct.
        struct FallbackFlagGuard {
            VirtualMachine* vm;
            bool prev;
            ~FallbackFlagGuard() { vm->inJitFallbackInterpreter = prev; }
        } guard{ this, inJitFallbackInterpreter };
        inJitFallbackInterpreter = true;

        return runJitMiniInterpret(savedIP, savedCallStackDepth, savedStackSize,
                                    /*savedProgram=*/nullptr,
                                    /*switchedProgram=*/false);
    }

    value::Value VirtualMachine::callMethodFromJit(
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
        const std::string& methodName,
        const std::vector<value::Value>& args)
    {
        if (!program || !instance)
        {
            throw errors::RuntimeException("JIT method fallback: invalid state");
        }

        auto classDef = instance->getClassDefinition();
        size_t argCount = args.size();

        auto lookupResult = classDef->findInstanceMethodForCallNameCached(methodName, argCount);
        if (!lookupResult.method)
        {
            std::string simpleMethodName = methodName;
            size_t colonPos = simpleMethodName.find("::");
            if (colonPos != std::string::npos)
            {
                simpleMethodName = simpleMethodName.substr(colonPos + 2);
            }
            size_t slashPos = simpleMethodName.find('/');
            if (slashPos != std::string::npos)
            {
                simpleMethodName = simpleMethodName.substr(0, slashPos);
            }
            throw errors::RuntimeException("Instance method not found: " + simpleMethodName +
                " with " + std::to_string(argCount) + " arguments in class " + classDef->getName());
        }

        std::string qualifiedName = lookupResult.qualifiedName;
        auto* funcMetadata = program->getFunction(qualifiedName);
        if (!funcMetadata)
        {
            throw errors::RuntimeException("Method '" + qualifiedName +
                "' has no bytecode.");
        }

        return callMethodFromJitDirect(instance, qualifiedName, funcMetadata, args);
    }

    value::Value VirtualMachine::callMethodFromJit(
        const value::Value& receiver,
        const std::string& methodName,
        const std::vector<value::Value>& args)
    {
        if (value::isObject(receiver))
        {
            return callMethodFromJit(value::asObject(receiver), methodName, args);
        }

        // MYT-208: STACK_OBJECT receiver. Resolve the method via the same
        // metadata path as the OBJECT branch, then dispatch through the
        // tag-aware Direct overload so the new ctor frame uses
        // thisInstanceRaw and the operand stack push preserves the tag.
        if (value::isStackObject(receiver))
        {
            auto* raw = value::asObjectInstanceRaw(receiver);
            if (!raw)
            {
                throw errors::RuntimeException("JIT method fallback: null STACK_OBJECT");
            }
            auto classDef = raw->getClassDefinition();
            size_t argCount = args.size();

            auto lookupResult = classDef->findInstanceMethodForCallNameCached(methodName, argCount);
            if (!lookupResult.method)
            {
                std::string simpleMethodName = methodName;
                size_t colonPos = simpleMethodName.find("::");
                if (colonPos != std::string::npos)
                {
                    simpleMethodName = simpleMethodName.substr(colonPos + 2);
                }
                size_t slashPos = simpleMethodName.find('/');
                if (slashPos != std::string::npos)
                {
                    simpleMethodName = simpleMethodName.substr(0, slashPos);
                }
                throw errors::RuntimeException("Instance method not found: " + simpleMethodName +
                    " with " + std::to_string(argCount) + " arguments in class " + classDef->getName());
            }

            std::string qualifiedName = lookupResult.qualifiedName;
            auto* funcMetadata = program->getFunction(qualifiedName);
            if (!funcMetadata)
            {
                throw errors::RuntimeException("Method '" + qualifiedName + "' has no bytecode.");
            }
            return callMethodFromJitDirect(receiver, qualifiedName, funcMetadata, args);
        }

        if (!value::isValueObject(receiver))
        {
            throw errors::RuntimeException(
                "JIT method fallback: receiver is neither ObjectInstance nor ValueObject");
        }

        auto valueObj = value::asValueObject(receiver);
        if (!valueObj)
        {
            throw errors::RuntimeException("JIT method fallback: null ValueObject");
        }
        auto classDef = valueObj->getClassDefinition();
        if (!classDef)
        {
            throw errors::RuntimeException(
                "JIT method fallback: ValueObject has no class definition");
        }

        // Value-class fast-dispatch fix: batch-materialise a temp ObjectInstance
        // from the ValueObject's field vector (no per-field setField hashmap
        // thrash), then reuse the regular-class path. In-method mutation
        // semantics stay identical to the pre-fix behaviour — the tempInstance
        // is independent of the caller's ValueObject, so `this.x = …` writes
        // mutate the temp in place and never propagate back.
        auto tempInstance = value::ObjectInstancePool::getInstance().acquire(classDef);
        tempInstance->loadFromValueObject(*valueObj);

        return callMethodFromJit(tempInstance, methodName, args);
    }

    value::Value VirtualMachine::callMethodFromJitDirect(
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
        const std::string& qualifiedName,
        const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata,
        const std::vector<value::Value>& args,
        const bytecode::BytecodeProgram* calleeProgram)
    {
        if (!program || !instance || !funcMetadata)
        {
            throw errors::RuntimeException("JIT method direct: invalid state");
        }

        // MYT-182: if the callee lives in a different program (library),
        // resolve its programIndex up front so the frame can carry it for
        // return restoration, and so the mini-interpret loop runs against
        // the correct bytecode.
        const bytecode::BytecodeProgram* savedProgram = executionCtx->program;
        size_t calleeProgramIndex = callStack.empty() ? 0 : callStack.back().programIndex;
        bool switchedProgram = false;
        if (calleeProgram && calleeProgram != savedProgram)
        {
            for (size_t i = 0; i < loadedPrograms.size(); ++i)
            {
                if (loadedPrograms[i] == calleeProgram)
                {
                    calleeProgramIndex = i;
                    break;
                }
            }
            executionCtx->program = calleeProgram;
            switchedProgram = true;
        }

        // Defining class is the prefix of the qualified name (matches the
        // interpretation path that walks the class hierarchy to find the
        // introducing class).
        std::string definingClassName = instance->getClassDefinition()->getName();
        size_t colonPos = qualifiedName.find("::");
        if (colonPos != std::string::npos)
        {
            definingClassName = qualifiedName.substr(0, colonPos);
        }

        size_t savedIP = instructionPointer;
        size_t savedCallStackDepth = callStack.size();
        size_t savedStackSize = stackManager->size();

        stackManager->push(instance);
        for (const auto& arg : args)
        {
            stackManager->push(arg);
        }
        size_t pushedSlots = 1 + args.size();
        for (size_t i = pushedSlots; i < funcMetadata->localCount; ++i)
        {
            stackManager->push(std::monostate{});
        }

        CallFrame frame;
        frame.returnAddress = savedIP;
        frame.frameBase = savedStackSize;
        frame.localBase = savedStackSize;
        // MYT-197: intern on the callee's owning program. Prefer calleeProgram
        // (passed by the JIT caller) when set; fall back to whichever program
        // calleeProgramIndex resolves to. Never index-dereference loadedPrograms
        // blindly — if the lookup above didn't find calleeProgram in the list,
        // calleeProgramIndex could point at the wrong program.
        const bytecode::BytecodeProgram* ownerProgram =
            calleeProgram ? calleeProgram
                          : (calleeProgramIndex < loadedPrograms.size()
                                 ? loadedPrograms[calleeProgramIndex]
                                 : program);
        frame.functionName = ownerProgram->internFrameName(qualifiedName);
        frame.thisInstance = instance;
        frame.definingClassName = definingClassName;
        // MYT-182: carry the callee's programIndex so the normal return path
        // in ControlFlowExecutor restores context.program correctly.
        frame.programIndex = calleeProgramIndex;
        pushCallFrame(std::move(frame));

        instructionPointer = funcMetadata->startOffset;

        return runJitMiniInterpret(savedIP, savedCallStackDepth, savedStackSize,
                                    savedProgram, switchedProgram);
    }

    value::Value VirtualMachine::callMethodFromJitDirect(
        const value::Value& receiver,
        const std::string& qualifiedName,
        const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata,
        const std::vector<value::Value>& args,
        const bytecode::BytecodeProgram* calleeProgram)
    {
        if (value::isObject(receiver))
        {
            return callMethodFromJitDirect(value::asObject(receiver), qualifiedName,
                                           funcMetadata, args, calleeProgram);
        }

        // MYT-208: STACK_OBJECT receiver. Same dispatch as the shared_ptr
        // overload but pushes the tagged Value (no shared_ptr copy) and
        // routes `this` through frame.thisInstanceRaw.
        if (value::isStackObject(receiver))
        {
            return callMethodFromJitDirectStack(receiver, qualifiedName,
                                                funcMetadata, args, calleeProgram);
        }

        if (!value::isValueObject(receiver))
        {
            throw errors::RuntimeException("JIT method direct: receiver kind not supported");
        }

        auto valueObj = value::asValueObject(receiver);
        if (!valueObj || !valueObj->getClassDefinition())
        {
            throw errors::RuntimeException("JIT method direct: null or classless ValueObject");
        }

        // Same batch-materialisation as callMethodFromJit(Value). Keeps the
        // MYT-184 stack-cookie invariant intact — we still land in the
        // shared_ptr<ObjectInstance> mini-interpret loop, not a direct
        // JIT→JIT dispatch.
        auto tempInstance = value::ObjectInstancePool::getInstance().acquire(valueObj->getClassDefinition());
        tempInstance->loadFromValueObject(*valueObj);

        return callMethodFromJitDirect(tempInstance, qualifiedName, funcMetadata, args, calleeProgram);
    }

    value::Value VirtualMachine::callMethodFromJitDirectStack(
        const value::Value& receiverValue,
        const std::string& qualifiedName,
        const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata,
        const std::vector<value::Value>& args,
        const bytecode::BytecodeProgram* calleeProgram)
    {
        // MYT-208: STACK_OBJECT-receiver counterpart to callMethodFromJitDirect:
        // raw `this` threaded through frame.thisInstanceRaw and the
        // STACK_OBJECT-tagged Value pushed as local-0. Lifetime of the borrowed
        // instance is owned by the caller's frame (via stackObjects), not by
        // anything pushed here.
        auto* raw = value::asObjectInstanceRaw(receiverValue);
        if (!program || !raw || !funcMetadata)
        {
            throw errors::RuntimeException("JIT method direct (stack): invalid state");
        }

        const bytecode::BytecodeProgram* savedProgram = executionCtx->program;
        size_t calleeProgramIndex = callStack.empty() ? 0 : callStack.back().programIndex;
        bool switchedProgram = false;
        if (calleeProgram && calleeProgram != savedProgram)
        {
            for (size_t i = 0; i < loadedPrograms.size(); ++i)
            {
                if (loadedPrograms[i] == calleeProgram)
                {
                    calleeProgramIndex = i;
                    break;
                }
            }
            executionCtx->program = calleeProgram;
            switchedProgram = true;
        }

        std::string definingClassName = raw->getClassDefinition()->getName();
        size_t colonPos = qualifiedName.find("::");
        if (colonPos != std::string::npos)
        {
            definingClassName = qualifiedName.substr(0, colonPos);
        }

        size_t savedIP = instructionPointer;
        size_t savedCallStackDepth = callStack.size();
        size_t savedStackSize = stackManager->size();

        // Push receiver Value (STACK_OBJECT-tagged) as local-0 — preserves the
        // tag for the callee's LOAD_LOCAL and keeps the operand stack
        // refcount-free. Then args, then null-fill remaining locals.
        stackManager->push(receiverValue);
        for (const auto& arg : args)
        {
            stackManager->push(arg);
        }
        size_t pushedSlots = 1 + args.size();
        for (size_t i = pushedSlots; i < funcMetadata->localCount; ++i)
        {
            stackManager->push(std::monostate{});
        }

        CallFrame frame;
        frame.returnAddress = savedIP;
        frame.frameBase = savedStackSize;
        frame.localBase = savedStackSize;
        const bytecode::BytecodeProgram* ownerProgram =
            calleeProgram ? calleeProgram
                          : (calleeProgramIndex < loadedPrograms.size()
                                 ? loadedPrograms[calleeProgramIndex]
                                 : program);
        frame.functionName = ownerProgram->internFrameName(qualifiedName);
        frame.thisInstanceRaw = raw;
        frame.definingClassName = definingClassName;
        frame.programIndex = calleeProgramIndex;
        pushCallFrame(std::move(frame));

        instructionPointer = funcMetadata->startOffset;

        return runJitMiniInterpret(savedIP, savedCallStackDepth, savedStackSize,
                                    savedProgram, switchedProgram);
    }
}
