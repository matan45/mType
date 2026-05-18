#include "VirtualMachine.hpp"
#include <cstddef>
#include <cstdint>
#include <cassert>
#include "../../errors/RuntimeException.hpp"
#include "../../errors/UserException.hpp"
#include "utils/ExceptionHandler.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../environment/registry/ClassDefinition.hpp"
#include "../../value/SmallArgsBuffer.hpp"
#include "../../value/ValueObject.hpp"
#include "../../value/ValueShim.hpp"
#include "../../value/ObjectInstancePool.hpp"
#include "../jit/JitProfiler.hpp"
#include "../jit/JitCodeCache.hpp"
#include "../jit/JitCompiler.hpp"
#include "../jit/JitContext.hpp"
#include "../jit/OSRManager.hpp"
#include "../jit/LoopProfiler.hpp"
#include "../jit/guards/DeoptimizationHandler.hpp"
#include "../jit/ic/InlineCacheTable.hpp"
#include "../jit/ic/TypeFeedbackCollector.hpp"
#include "executors/FunctionExecutor.hpp"
#include "executors/ArrayExecutor.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../bytecode/OpCode.hpp"
#include "../../environment/Environment.hpp"
#include "../../environment/registry/ClassRegistry.hpp"
#include <algorithm>
#include <exception>
#include <iostream>

namespace vm::runtime
{
    namespace
    {
        // MYT-268: shared deopt-detection idiom for executeCallWithJit /
        // executeCallFastWithJit. jit_await stashes OSRDeoptException on
        // ctx->pendingException instead of throwing (the throw form crashed
        // silently on Windows x64 — no PE unwind data registered for the
        // asmjit frame). std::exception_ptr can't be peeked without
        // rethrowing; this helper rethrows once to inspect the type, and
        // either reports "deopt sentinel" or rethrows real errors so the
        // VM loop sees them exactly as the previous direct rethrow did.
        bool isStashedOSRDeopt(std::exception_ptr ep)
        {
            try
            {
                std::rethrow_exception(ep);
            }
            catch (const jit::OSRDeoptException&)
            {
                return true;
            }
            catch (...)
            {
                std::rethrow_exception(ep);
            }
            return false; // unreachable
        }
    }

    value::Value VirtualMachine::runJitMiniInterpret(
        size_t savedIP,
        size_t savedCallStackDepth,
        size_t savedStackSize,
        const bytecode::BytecodeProgram* savedProgram,
        bool switchedProgram)
    {
        // This function runs its own interpreter loop on the native C++
        // stack so it contributes to native recursion depth. Caller has
        // already pushed the frame and set instructionPointer.
        ++jitNativeDepth;

        // Use executionCtx->program so cross-library calls fetch from the
        // correct bytecode. Re-read each iteration via the reference so an
        // executor that switches program (e.g. cross-library CALL) is
        // honoured by the next instruction fetch.
        auto& jitCurrentProgram = executionCtx->program;
        while (callStack.size() > savedCallStackDepth)
        {
            if (instructionPointer >= jitCurrentProgram->getInstructionCount())
            {
                break;
            }

            const auto& instr = jitCurrentProgram->getInstruction(instructionPointer);

            try
            {
                executeInstruction(instr);
            }
            catch (errors::UserException& e)
            {
                // MYT-254: dispatch user exceptions to a catch handler in the
                // current frame, mirroring VirtualMachineLoop's main-loop
                // handling. Without this the exception bubbles out of the
                // mini-interpret loop, gets caught at the JIT helper boundary
                // (jit_call_method_ic stores it on ctx->pendingException), and
                // the surrounding OSR loop's back-edge check rethrows it past
                // the user's try/catch — turning a perfectly handlable in-
                // function throw into an uncaught escape (try_catch_finally_hot
                // regression). Pre-check: only call handleUserException when
                // the top frame has a covering handler entry, so we never
                // unwind past savedCallStackDepth (handleUserException can pop
                // frames as it searches; we don't want to lose our entry frame).
                bool dispatchInScope = false;
                if (!callStack.empty() && callStack.size() > savedCallStackDepth)
                {
                    auto* funcMeta = jitCurrentProgram->getFunctionMeta(
                        callStack.back().functionName);
                    if (funcMeta && funcMeta->exceptionTable.size() > 0)
                    {
                        const auto* handler = funcMeta->exceptionTable.findHandler(
                            instructionPointer,
                            e.getExceptionTypeName(),
                            e.getExceptionValue());
                        if (handler) dispatchInScope = true;
                    }
                }
                if (dispatchInScope && exceptionHandler)
                {
                    auto result = exceptionHandler->handleUserException(
                        e, instructionPointer, currentFinallyOffset);
                    if (result.handled
                        && callStack.size() > savedCallStackDepth)
                    {
                        instructionPointer = result.newInstructionPointer;
                        if (result.jumpedToFinally)
                        {
                            pendingException =
                                std::make_unique<errors::UserException>(e);
                        }
                        continue;
                    }
                }
                // No in-scope handler — restore state and rethrow so the
                // outer interpreter's main loop catches and dispatches.
                --jitNativeDepth;
                while (callStack.size() > savedCallStackDepth)
                {
                    callStack.back().releaseStackObjects();
                    callStack.pop_back();
                }
                instructionPointer = savedIP;
                while (stackManager->size() > savedStackSize)
                {
                    stackManager->pop();
                }
                if (switchedProgram)
                {
                    executionCtx->program = savedProgram;
                }
                throw;
            }
            catch (...)
            {
                --jitNativeDepth;
                // Restore state on exception. throw; below preserves the
                // original exception object (type + what()) intact.
                while (callStack.size() > savedCallStackDepth)
                {
                    // MYT-208: release stack-promoted allocations before pop.
                    callStack.back().releaseStackObjects();
                    callStack.pop_back();
                }
                instructionPointer = savedIP;
                while (stackManager->size() > savedStackSize)
                {
                    stackManager->pop();
                }
                // MYT-182: restore the pre-call program on exception too.
                if (switchedProgram)
                {
                    executionCtx->program = savedProgram;
                }
                throw;
            }

            // Post-increment to match interpretLoop pattern (executors set IP = target - 1).
            instructionPointer++;
        }

        --jitNativeDepth;

        // Get return value (if any)
        value::Value result = std::monostate{};
        if (stackManager->size() > savedStackSize)
        {
            result = stackManager->pop();
        }

        // Clean up stack to original size
        while (stackManager->size() > savedStackSize)
        {
            stackManager->pop();
        }

        // Restore instruction pointer
        instructionPointer = savedIP;

        // MYT-182: restore the pre-call program. The mini-interpret loop
        // exits on frame-depth check rather than running a RETURN handler,
        // so restore explicitly here.
        if (switchedProgram)
        {
            executionCtx->program = savedProgram;
        }

        return result;
    }

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

        // Push arguments, then null-fill the remaining locals.
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

        // Push 'this' as local-0, then args, then null-fill remaining locals.
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
        frame.thisInstanceRaw = raw;  // STACK_OBJECT path
        frame.definingClassName = definingClassName;
        frame.programIndex = calleeProgramIndex;
        pushCallFrame(std::move(frame));

        instructionPointer = funcMetadata->startOffset;

        return runJitMiniInterpret(savedIP, savedCallStackDepth, savedStackSize,
                                    savedProgram, switchedProgram);
    }

    void VirtualMachine::trySpecializeArithmetic(
        const bytecode::BytecodeProgram::Instruction& instr,
        bytecode::OpCode intOpcode, bytecode::OpCode floatOpcode)
    {
        if (!icEnabled || !typeFeedbackCollector || stackManager->size() < 2)
            return;

        typeFeedbackCollector->recordBinaryOp(
            instructionPointer, stackManager->peek(1), stackManager->peek(0));

        if (!typeFeedbackCollector->shouldSpecialize(instructionPointer))
            return;

        auto [lt, rt] = typeFeedbackCollector->getDominantTypes(instructionPointer);
        if (lt == jit::ic::ObservedType::INT && rt == jit::ic::ObservedType::INT)
        {
            const_cast<bytecode::BytecodeProgram::Instruction&>(instr).opcode = intOpcode;
            inlineCacheTable->getTypeFeedback(instructionPointer).specialized = true;

            // MYT-198: try to fuse ADD_INT with a preceding PUSH_INT into
            // ADD_INT_CONST. Only ADD_INT is covered by the fused opcode set
            // for MVP (the same pattern would extend to SUB/MUL/DIV trivially).
            if (intOpcode == bytecode::OpCode::ADD_INT)
            {
                tryFuseAddIntConst();
            }
        }
        else if (lt == jit::ic::ObservedType::FLOAT && rt == jit::ic::ObservedType::FLOAT)
        {
            const_cast<bytecode::BytecodeProgram::Instruction&>(instr).opcode = floatOpcode;
            inlineCacheTable->getTypeFeedback(instructionPointer).specialized = true;
        }
    }

    void VirtualMachine::trySpecializeBitwise(
        const bytecode::BytecodeProgram::Instruction& instr,
        bytecode::OpCode intOpcode)
    {
        if (!icEnabled || !typeFeedbackCollector || stackManager->size() < 2)
            return;

        // Sticky-demote gate. The TypeFeedback.specialized flag already
        // blocks re-entry via shouldSpecialize, but if any future demote
        // path clears it (mirroring MYT-173 CALL_METHOD_CACHED deopts), the
        // cachedDeoptCount check prevents oscillation on a site that has
        // already un-specialized once.
        if (auto* existing = program->findCachedState(instructionPointer))
        {
            if (existing->cachedDeoptCount >= 1) return;
        }

        typeFeedbackCollector->recordBinaryOp(
            instructionPointer, stackManager->peek(1), stackManager->peek(0));

        if (!typeFeedbackCollector->shouldSpecialize(instructionPointer))
            return;

        auto [lt, rt] = typeFeedbackCollector->getDominantTypes(instructionPointer);
        if (lt == jit::ic::ObservedType::INT && rt == jit::ic::ObservedType::INT)
        {
            const_cast<bytecode::BytecodeProgram::Instruction&>(instr).opcode = intOpcode;
            inlineCacheTable->getTypeFeedback(instructionPointer).specialized = true;
        }
    }

    void VirtualMachine::trySpecializeBitwiseUnary(
        const bytecode::BytecodeProgram::Instruction& instr,
        bytecode::OpCode intOpcode)
    {
        if (!icEnabled || !typeFeedbackCollector || stackManager->size() < 1)
            return;

        if (auto* existing = program->findCachedState(instructionPointer))
        {
            if (existing->cachedDeoptCount >= 1) return;
        }

        // Reuse recordBinaryOp with the same operand twice — the feedback
        // collector only inspects tag, and duplicating avoids adding a
        // separate unary path for a one-operand promotion.
        const value::Value& tos = stackManager->peek(0);
        typeFeedbackCollector->recordBinaryOp(instructionPointer, tos, tos);

        if (!typeFeedbackCollector->shouldSpecialize(instructionPointer))
            return;

        auto [lt, rt] = typeFeedbackCollector->getDominantTypes(instructionPointer);
        if (lt == jit::ic::ObservedType::INT && rt == jit::ic::ObservedType::INT)
        {
            const_cast<bytecode::BytecodeProgram::Instruction&>(instr).opcode = intOpcode;
            inlineCacheTable->getTypeFeedback(instructionPointer).specialized = true;
        }
    }

    void VirtualMachine::tryFuseAddIntConst()
    {
        auto* activeProgram = executionCtx ? executionCtx->program : program;
        if (!activeProgram) return;

        const size_t ip = instructionPointer;
        if (ip == 0) return;

        // Prior must be PUSH_INT; its operand[0] is a constant-pool index
        // (see StackOperationsExecutor::handlePushInt). ADD_INT_CONST stores
        // that same index in fusedSlot and reads the literal from the pool
        // at dispatch time — no eager resolution.
        const auto& prev = activeProgram->getInstruction(ip - 1);
        if (prev.opcode != bytecode::OpCode::PUSH_INT) return;
        if (prev.numOperands() == 0) return;

        // Same fusion-safety gates as the LOAD_LOCAL fusions: no control-flow
        // target may land directly on the fused op, and sticky un-fuse blocks
        // re-fusion.
        if (activeProgram->isFusionUnsafeTarget(ip)) return;

        // MYT-201: fused state lives in the per-IP side table. Sticky un-fuse
        // gate reads via findCachedState so a never-fused site doesn't
        // allocate an entry just to check the default 0.
        if (auto* existing = activeProgram->findCachedState(ip))
        {
            if (existing->fusedDeoptCount >= 1) return;
        }

        uint64_t constIdx = prev.inlineOperands[0];
        // fusedSlot is uint32_t — assert before truncation so an oversized
        // constant-pool index surfaces as a clean failure rather than silent
        // data loss. Constant pools are bounded by the compiler, but the
        // cast is attacker-reachable via a malformed .mtc operand.
        assert(constIdx <= UINT32_MAX &&
               "ADD_INT_CONST fusion: PUSH_INT operand exceeds fusedSlot width");
        auto& prevMut = const_cast<bytecode::BytecodeProgram*>(activeProgram)
                            ->getMutableInstruction(ip - 1);
        prevMut.opcode = bytecode::OpCode::NOP;
        prevMut.clearOperands();

        auto& state = activeProgram->getOrCreateCachedState(ip);
        state.fusedSlot = static_cast<uint32_t>(constIdx);

        auto& mut = const_cast<bytecode::BytecodeProgram*>(activeProgram)
                        ->getMutableInstruction(ip);
        mut.opcode = bytecode::OpCode::ADD_INT_CONST;
    }

    void VirtualMachine::executeCallWithJit(
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        using OpCode = bytecode::OpCode;

        if (jitEnabled && jitCodeCache && jitNativeDepth < MAX_JIT_NATIVE_DEPTH
            && !inJitFallbackInterpreter)
        {
            const auto* activeProgram = executionCtx ? executionCtx->program : program;
            std::string funcName = activeProgram->getConstantPool().getString(instr.inlineOperands[0]);
            auto jitCode = jitCodeCache->lookup(funcName);
            if (jitCode)
            {
                size_t argCount = instr.inlineOperands[1];

                // MYT-196: small-buffer-optimized args for JIT entry.
                value::SmallArgsBuffer args(argCount);
                for (size_t i = argCount; i > 0; --i)
                    args[i - 1] = stackManager->pop();

                jit::JitContext jitCtx{};
                jitCtx.args = args.data();
                jitCtx.argCount = args.size();
                jitCtx.hasReturnValue = false;
                jitCtx.program = activeProgram;
                jitCtx.stackManager = stackManager.get();
                jitCtx.environment = environment.get();
                jitCtx.vm = this;
                jitCtx.jitCodeCache = jitCodeCache.get();
                jitCtx.icTable = inlineCacheTable.get();

                size_t sepPos = funcName.find("::");
                if (sepPos != std::string::npos)
                    jitCtx.callingClassName = funcName.substr(0, sepPos);

                ++jitNativeDepth;
                jitCode(&jitCtx);
                --jitNativeDepth;

                // MYT-268: JIT-AWAIT deopt landed at the function-level
                // boundary. The JIT body uses asmjit-private locals /
                // operand-stack, so we can't materialize its mid-body
                // state into the interpreter cleanly. Re-execute the
                // call from scratch in the interpreter. Side-effect-free
                // bodies (the async benchmarks, which never deopt anyway
                // because they hit PROMISE_INT inline) re-execute
                // cleanly. Bodies with externally visible side effects
                // before deopt may double-execute them — known v1
                // limitation. isStashedOSRDeopt rethrows non-deopt
                // exceptions to the VM loop unchanged.
                if (jitCtx.pendingException
                    && isStashedOSRDeopt(jitCtx.pendingException))
                {
                    for (size_t i = 0; i < args.size(); ++i)
                        stackManager->push(args[i]);
                    functionExecutor->handleCall(instr);
                    stats.functionCalls++;
                    return;
                }

                if (jitCtx.hasReturnValue)
                    stackManager->push(jitCtx.returnValue);

                stats.functionCalls++;
                return;
            }
        }
        // Fall back to interpreter (uses managed call stack with overflow protection)
        functionExecutor->handleCall(instr);
    }

    void VirtualMachine::executeCallFastWithJit(
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (jitEnabled && jitCodeCache && jitNativeDepth < MAX_JIT_NATIVE_DEPTH
            && !inJitFallbackInterpreter)
        {
            const auto* activeProgram = executionCtx ? executionCtx->program : program;
            const auto* funcMeta = activeProgram->getFunctionByIndex(instr.inlineOperands[0]);
            if (funcMeta) {
                auto jitCode = jitCodeCache->lookup(funcMeta->mangledName);
                if (jitCode)
                {
                    size_t argCount = instr.inlineOperands[1];

                    // MYT-196: small-buffer-optimized args for JIT entry.
                    value::SmallArgsBuffer args(argCount);
                    for (size_t i = argCount; i > 0; --i)
                        args[i - 1] = stackManager->pop();

                    jit::JitContext jitCtx{};
                    jitCtx.args = args.data();
                    jitCtx.argCount = args.size();
                    jitCtx.hasReturnValue = false;
                    jitCtx.program = activeProgram;
                    jitCtx.stackManager = stackManager.get();
                    jitCtx.environment = environment.get();
                    jitCtx.vm = this;
                    jitCtx.jitCodeCache = jitCodeCache.get();
                    jitCtx.icTable = inlineCacheTable.get();

                    size_t sepPos = funcMeta->name.find("::");
                    if (sepPos != std::string::npos)
                        jitCtx.callingClassName = funcMeta->name.substr(0, sepPos);

                    ++jitNativeDepth;
                    jitCode(&jitCtx);
                    --jitNativeDepth;

                    // MYT-268: see executeCallWithJit for rationale.
                    if (jitCtx.pendingException
                        && isStashedOSRDeopt(jitCtx.pendingException))
                    {
                        for (size_t i = 0; i < args.size(); ++i)
                            stackManager->push(args[i]);
                        functionExecutor->handleCallFast(instr);
                        stats.functionCalls++;
                        return;
                    }

                    if (jitCtx.hasReturnValue)
                        stackManager->push(jitCtx.returnValue);

                    stats.functionCalls++;
                    return;
                }
            }
        }
        functionExecutor->handleCallFast(instr);
    }

    void VirtualMachine::invalidateInlinedFunctionCallers(
        bytecode::FunctionNameHandle callee)
    {
        if (!jitCodeCache) return;
        auto callers = jitCodeCache->invalidatedInlineCallersOf(callee);
        if (callers.empty()) return;

        auto* icTable = inlineCacheTable.get();
        for (const auto& callerName : callers)
        {
            // JitCodeCache::invalidate releases the native code memory and
            // returns the JitFunction pointer that was freed. The MYT-315
            // contract requires us to scrub every IC table that may hold
            // that pointer.
            jit::JitFunction removed = jitCodeCache->invalidate(callerName);
            if (!removed) continue;
            const void* removedPtr = reinterpret_cast<const void*>(removed);
            if (icTable)
            {
                icTable->clearCachedJitForFunction(removedPtr);
            }
            // MYT-322: also scrub free-function IC side-tables on every
            // loaded program. A library callee invalidated here may have
            // cachedJitFnPtr entries in caller programs' CachedInstructionState
            // maps; those tables live per-BytecodeProgram, not in the
            // global ICTable, so we must visit them all to avoid jumping
            // into freed native code on the next warm dispatch.
            for (const auto* prog : getLoadedPrograms())
            {
                if (prog) prog->clearCachedJitFnPtrFor(removedPtr);
            }
            // A caller F that inlined `callee` may itself have been inlined
            // into a third caller G. We don't have F's handle directly here
            // — the next compile that pastes F into G will register a fresh
            // edge under F's handle. For correctness today we rely on the
            // current invalidate target chain (PluginLoader walks all
            // unbound names; REPL redefinition passes the redefined
            // function's handle). If transitive eviction becomes necessary,
            // resolve callerName -> handle via internFrameName and recurse.
        }
    }

    void VirtualMachine::printJitStats() const
    {
        std::cout << "\n=== JIT Statistics ===\n";

        if (!jitEnabled)
        {
            std::cout << "  JIT disabled\n";
            std::cout << "======================\n";
            return;
        }

        // Function profiling
        std::cout << "Function Profiling:\n";
        if (jitProfiler)
        {
            std::cout << "  Hot threshold:          " << jitProfiler->getHotThreshold() << " calls\n";
            const auto& hotFuncs = jitProfiler->getHotFunctions();
            std::cout << "  Hot functions:          " << hotFuncs.size() << "\n";
            for (const auto& name : hotFuncs)
            {
                uint32_t calls = jitProfiler->getInvocationCount(name);
                bool compiled = jitCodeCache && jitCodeCache->contains(name);
                std::cout << "    - " << name << " (" << calls << " calls)"
                          << (compiled ? " [compiled]" : " [bailout]") << "\n";
            }
        }

        // Compilation stats
        std::cout << "Compilation:\n";
        if (jitCompiler)
        {
            std::cout << "  Successful compiles:    " << jitCompiler->getCompileCount() << "\n";
            std::cout << "  Bailouts:               " << jitCompiler->getBailoutCount() << "\n";
            auto printOpcodeBailouts = [](const char* label,
                                          const std::array<uint64_t, 256>& counts)
            {
                bool any = false;
                for (uint64_t count : counts)
                {
                    if (count != 0) { any = true; break; }
                }
                if (!any) return;

                std::cout << label << "\n";
                for (size_t i = 0; i < counts.size(); ++i)
                {
                    if (counts[i] == 0) continue;
                    std::cout << "    - "
                              << bytecode::getOpCodeName(static_cast<bytecode::OpCode>(i))
                              << ": " << counts[i] << "\n";
                }
            };
            printOpcodeBailouts("  Function bailout opcodes:",
                                jitCompiler->getFunctionBailoutOpcodes());
            printOpcodeBailouts("  OSR bailout opcodes:",
                                jitCompiler->getOSRBailoutOpcodes());
            // MYT-207: self-recursion call-dispatch telemetry.
            //   Tail calls optimized — `return self(args...)` sites lowered to
            //     arg-overwrite + jmp (loop). Counts CALL SITES, not dynamic
            //     iterations.
            //   Self direct calls — non-tail self-recursive sites lowered to
            //     a direct asmjit invoke against funcNode->label(), skipping
            //     the jit_call_function dispatch overhead.
            std::cout << "  Tail calls optimized:   " << jitCompiler->getTailCallsOptimized() << "\n";
            std::cout << "  Self direct calls:      " << jitCompiler->getSelfDirectCalls() << "\n";
        }
        if (jitCodeCache)
            std::cout << "  Cached functions:       " << jitCodeCache->size() << "\n";

        // MYT-172 AC #3: inline field-IC stats. Hits = JIT fast-path took
        // the inlined shape-guard + indexed load/store. Misses = shape
        // guard failed and the slow-path helper handled the access.
        // MYT-191: SET counters split out separately — GET misses are
        // dominated by non-primitive tags, SET misses by ValueObject CoW,
        // merging them hides regressions.
        if (jitCompiler)
        {
            std::cout << "Field IC (GET):\n";
            std::cout << "  Inline hits:            "
                      << jitCompiler->getInlineFieldICHits() << "\n";
            std::cout << "  Inline misses:          "
                      << jitCompiler->getInlineFieldICMisses() << "\n";
            std::cout << "Field IC (SET):\n";
            std::cout << "  Inline hits:            "
                      << jitCompiler->getInlineFieldSetICHits() << "\n";
            std::cout << "  Inline misses:          "
                      << jitCompiler->getInlineFieldSetICMisses() << "\n";

            // MYT-210 (fills MYT-179 stub): per-decision inline-eligibility
            // breakdown for both method-call and plain-function inlining.
            // Print only non-zero rows to keep output compact; if every row
            // is zero (no JIT-compiled function reached an inlineable site),
            // skip the section entirely.
            const auto& dec = jitCompiler->getInlineDecisions();
            auto anyNonZero = [](const std::array<uint64_t, jit::InlineDecisionCounters::SIZE>& a) {
                for (auto v : a) if (v) return true;
                return false;
            };
            auto printSection = [](const char* label,
                                    const std::array<uint64_t, jit::InlineDecisionCounters::SIZE>& a) {
                std::cout << label << "\n";
                for (size_t i = 0; i < jit::InlineDecisionCounters::SIZE; ++i)
                {
                    if (!a[i]) continue;
                    const auto reason = static_cast<optimization::InlineDecision>(i);
                    std::cout << "  " << optimization::inlineDecisionName(reason)
                              << ": " << a[i] << "\n";
                }
            };
            if (anyNonZero(dec.perReasonMethod))
                printSection("Inline decisions (method):", dec.perReasonMethod);
            if (anyNonZero(dec.perReasonFunction))
                printSection("Inline decisions (function):", dec.perReasonFunction);
        }

        // Loop OSR stats
        std::cout << "Loop OSR:\n";
        if (osrManager)
        {
            const auto& loopProfiler = osrManager->getLoopProfiler();
            const auto& profiles = loopProfiler.getProfiles();
            size_t compiled = 0, failed = 0;
            for (const auto& [id, profile] : profiles)
            {
                if (profile.osrCompiled) compiled++;
                if (profile.osrFailed) failed++;
            }
            std::cout << "  OSR threshold:          " << loopProfiler.getOsrThreshold() << " iterations\n";
            std::cout << "  Loops profiled:         " << profiles.size() << "\n";
            std::cout << "  OSR compiled:           " << compiled << "\n";
            std::cout << "  OSR failed:             " << failed << "\n";

            // MYT-148: per-loop bailout reason breakdown. Answers "why didn't
            // this loop tier up?" without a debugger. Shows the offending
            // opcode mnemonic for UNSUPPORTED_OPCODE / CODEGEN_FAILURE so the
            // next step (remediation) is obvious.
            if (failed > 0)
            {
                std::cout << "  Failed loops:\n";
                for (const auto& [id, profile] : profiles)
                {
                    if (!profile.osrFailed) continue;
                    std::cout << "    - offset 0x" << std::hex << id.jumpBackOffset
                              << std::dec << ": "
                              << jit::osrBailoutReasonName(profile.bailoutReason);
                    if (profile.bailoutReason == jit::OSRBailoutReason::UNSUPPORTED_OPCODE ||
                        profile.bailoutReason == jit::OSRBailoutReason::CODEGEN_FAILURE)
                    {
                        std::cout << " (0x" << std::hex
                                  << static_cast<unsigned>(profile.offendingOpcode)
                                  << std::dec << " = "
                                  << bytecode::getOpCodeName(
                                         static_cast<bytecode::OpCode>(profile.offendingOpcode))
                                  << ")";
                    }
                    std::cout << "\n";
                }
            }
        }

        std::cout << "======================\n";
    }

    value::Value VirtualMachine::createMultiArrayFromJit(uint32_t typeNameIndex,
                                                          const std::vector<int64_t>& dimensions,
                                                          size_t totalDimensions)
    {
        if (!program)
        {
            throw errors::RuntimeException("JIT multi-array fallback: no program loaded");
        }
        const std::string& elementTypeName =
            program->getConstantPool().getString(typeNameIndex);
        auto classRegistry = environment ? environment->getClassRegistry().get() : nullptr;
        return ArrayExecutor::buildMultiArray(classRegistry, elementTypeName, dimensions, totalDimensions);
    }
}
