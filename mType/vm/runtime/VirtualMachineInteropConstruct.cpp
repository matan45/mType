#include "VirtualMachine.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "executors/ControlFlowExecutor.hpp"
#include "utils/InteropExceptionDecorator.hpp"
#include "utils/ExceptionHandler.hpp"
#include "../../errors/ClassNotFoundException.hpp"
#include "../../errors/ConstructorNotFoundException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/ScriptException.hpp"
#include "../../errors/UserException.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../value/AsyncPromiseValue.hpp"
#include "../../value/ObjectInstancePool.hpp"
#include "../../value/PromiseValue.hpp"
#include "../../value/ValueShim.hpp"  // MYT-208: makeStackObjectValue
#include "context/SharedStackFramePool.hpp"

namespace vm::runtime
{
    namespace
    {
        std::vector<std::string> parseTypeArguments(const std::string& typeArgsStr)
        {
            std::vector<std::string> typeArgs;
            std::string current;
            int depth = 0;

            for (char c : typeArgsStr)
            {
                if (c == '<')
                {
                    depth++;
                    current += c;
                }
                else if (c == '>')
                {
                    depth--;
                    current += c;
                }
                else if (c == ',' && depth == 0)
                {
                    size_t start = current.find_first_not_of(" \t");
                    size_t end = current.find_last_not_of(" \t");
                    if (start != std::string::npos && end != std::string::npos)
                    {
                        typeArgs.push_back(current.substr(start, end - start + 1));
                    }
                    current.clear();
                }
                else
                {
                    current += c;
                }
            }

            size_t start = current.find_first_not_of(" \t");
            size_t end = current.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos)
            {
                typeArgs.push_back(current.substr(start, end - start + 1));
            }

            return typeArgs;
        }

        std::string parseGenericTypeName(
            const std::string& fullClassName,
            const std::shared_ptr<environment::Environment>& environment,
            std::unordered_map<std::string, std::string>& genericTypeBindings)
        {
            size_t genericStart = fullClassName.find('<');
            if (genericStart == std::string::npos)
            {
                return fullClassName;
            }

            std::string baseClassName = fullClassName.substr(0, genericStart);
            size_t genericEnd = fullClassName.rfind('>');
            if (genericEnd == std::string::npos || genericEnd <= genericStart)
            {
                return baseClassName;
            }

            auto classDef = environment ? environment->findClass(baseClassName) : nullptr;
            if (!classDef)
            {
                return baseClassName;
            }

            std::string typeArgsStr = fullClassName.substr(
                genericStart + 1, genericEnd - genericStart - 1);
            std::vector<std::string> typeArgs = parseTypeArguments(typeArgsStr);
            const auto& genericParams = classDef->getGenericParameters();
            for (size_t i = 0; i < genericParams.size() && i < typeArgs.size(); ++i)
            {
                genericTypeBindings[genericParams[i].name] = typeArgs[i];
            }

            return baseClassName;
        }
    }

    // Object construction & lambda invocation
    value::Value VirtualMachine::createObject(const std::string& className, std::span<const value::Value> args)
    {
        // Save current state up front so the typed catch can decorate the
        // exception against the LIVE callStack before we restore.
        size_t savedIP = instructionPointer;
        std::vector<CallFrame> savedCallStack = callStack;
        size_t savedCurrentFinallyOffset = currentFinallyOffset;

        try
        {
            if (!program)
            {
                throw errors::RuntimeException(
                    "No program loaded - cannot create object in bytecode mode without compiled bytecode");
            }

            std::unordered_map<std::string, std::string> genericTypeBindings;
            std::string baseClassName = parseGenericTypeName(
                className, environment, genericTypeBindings);

            // Find class definition
            auto classDef = environment->findClass(baseClassName);
            if (!classDef)
            {
                throw errors::ClassNotFoundException(className);
            }

            // Create object instance
            auto instance = genericTypeBindings.empty()
                ? value::ObjectInstancePool::getInstance().acquire(classDef)
                : value::ObjectInstancePool::getInstance().acquire(classDef, genericTypeBindings);

            // Find constructor using type-aware lookup
            auto constructor = classDef->findConstructorByTypes(args);
            if (!constructor)
            {
                // Check for default constructor case (no params, no explicit constructor)
                bool hasAnyConstructor = !classDef->getConstructors().empty();
                if (args.empty() && !hasAnyConstructor)
                {
                    // Default constructor - just return the instance
                    return value::Value(instance);
                }
                // MYT-46: typed throw → MT-E1009 NameConstructorNotFound.
                throw errors::ConstructorNotFoundException(className, args.size());
            }

            // Phase 3 (allocation perf): trivial ctor — `this.F_k = param_k`
            // only, no super-initialiser, all fields own-class. Copy args
            // directly into instance fields and skip the CallFrame + bytecode
            // interpret loop entirely. Compiler flags this at class-compile
            // time in ClassCompiler::markTrivialConstructors.
            //
            // Phase 4: use pre-resolved field indices + setFieldByIndex to
            // bypass string-hashed fieldValues::insert on every write.
            if (constructor->isTrivialConstructor())
            {
                const auto& indexed = constructor->getTrivialFieldIndexAssignments();
                instance->ensureFieldVector();
                for (const auto& [fieldIdx, paramIdx] : indexed)
                {
                    if (paramIdx < args.size())
                    {
                        instance->setFieldByIndex(fieldIdx, args[paramIdx]);
                    }
                }
                return value::Value(instance);
            }

            // Phase 2b (allocation perf): resolve constructor bytecode + frame
            // name once per (ctor, program) pair and cache on ConstructorDefinition.
            // Eliminates a per-call string concat + getFunction hashmap hit +
            // internFrameName hashmap hit — hot for jit_new_object → createObject.
            auto& cache = constructor->getCallSiteCache();
            const bytecode::BytecodeProgram::FunctionMetadata* ctorMetadata = nullptr;
            bytecode::FunctionNameHandle frameNameHandle{cache.frameNameHandle};

            if (cache.programTag == program && cache.funcMetadata != nullptr)
            {
                ctorMetadata = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(
                    cache.funcMetadata);
            }
            else
            {
                std::string typeSignature = constructor->getTypeSignature();
                std::string qualifiedName = typeSignature.empty()
                    ? baseClassName + "::<init>"
                    : baseClassName + "::<init>/" + typeSignature;
                ctorMetadata = program->getFunction(qualifiedName);
                if (!ctorMetadata)
                {
                    throw errors::RuntimeException("Constructor '" + qualifiedName +
                        "' has no bytecode. Bytecode compilation is required for VM execution.");
                }
                frameNameHandle = program->internFrameName(qualifiedName);
                cache.programTag = program;
                cache.funcMetadata = ctorMetadata;
                cache.frameNameHandle = frameNameHandle.id;
            }

            // Reset finally offset for new function call
            currentFinallyOffset = SIZE_MAX;

            // Push instance and arguments onto stack
            size_t frameBase = stackManager->size();
            push(instance);
            for (const auto& arg : args)
            {
                push(arg);
            }

            // Create call frame
            CallFrame frame;
            // Set return address to end of program so interpretLoop exits after constructor returns
            frame.returnAddress = program->getInstructionCount();
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            frame.functionName = frameNameHandle;
            frame.thisInstance = instance;
            frame.definingClassName = baseClassName;

            pushCallFrame(std::move(frame));
            stats.functionCalls++;

            // Execute constructor
            // Use direct execution loop instead of interpretLoop() to avoid
            // recreating executors (which would invalidate the outer ExecutionContext)
            instructionPointer = ctorMetadata->startOffset;
            if (controlFlowExecutor)
            {
                // Use executionCtx->program so cross-library calls fetch from the correct bytecode
                auto& ctorCurrentProgram = executionCtx->program;
                size_t targetDepth = savedCallStack.size();
                while (callStack.size() > targetDepth)
                {
                    if (instructionPointer >= ctorCurrentProgram->getInstructionCount())
                        break;
                    const auto& instr = ctorCurrentProgram->getInstruction(instructionPointer);
                    try
                    {
                        executeInstruction(instr);
                    }
                    catch (errors::UserException& e)
                    {
                        auto handlerResult = exceptionHandler->handleUserException(
                            e, instructionPointer, currentFinallyOffset);
                        if (!handlerResult.handled)
                        {
                            throw;
                        }
                        // MYT-111: handler found the catch in a caller frame
                        // above our reflective boundary. Roll back its mutations
                        // so the outer VM loop's own handler runs fresh.
                        if (callStack.size() < targetDepth)
                        {
                            callStack = savedCallStack;
                            while (stackManager->size() > frameBase)
                            {
                                stackManager->pop();
                            }
                            instructionPointer = savedIP;
                            currentFinallyOffset = savedCurrentFinallyOffset;
                            throw;
                        }
                        instructionPointer = handlerResult.newInstructionPointer;
                        if (handlerResult.jumpedToFinally)
                        {
                            pendingException = std::make_unique<errors::UserException>(e);
                        }
                        continue;
                    }
                    instructionPointer++;
                }
                // Clean up stack to original position
                while (stackManager->size() > frameBase)
                    stackManager->pop();
            }
            else
            {
                interpretLoop();
            }

            // Restore state
            instructionPointer = savedIP;
            callStack = savedCallStack;
            currentFinallyOffset = savedCurrentFinallyOffset;

            return instance;
        }
        catch (errors::ScriptException& e)
        {
            // MYT-46: decorate using LIVE callStack BEFORE state restore.
            utils::decorateFromCallStack(e, callStack, program);
            instructionPointer = savedIP;
            callStack = savedCallStack;
            currentFinallyOffset = savedCurrentFinallyOffset;
            throw;
        }
        catch (...)
        {
            // Restore state on error
            instructionPointer = savedIP;
            callStack = savedCallStack;
            currentFinallyOffset = savedCurrentFinallyOffset;
            throw;
        }
    }

    value::Value VirtualMachine::createStackObject(const std::string& className,
                                                   std::span<const value::Value> args)
    {
        // MYT-208: trivial-ctor fast path is what JIT-emitted NEW_STACK
        // primarily targets (Point(int, int) etc.). Non-trivial ctors fall
        // back to heap to keep the v1 small — running an interpreter ctor
        // body with raw `this` requires the same frame-setup machinery as
        // ObjectInstanceHelper::handleNewStack, which the interpreter side
        // already does. JIT-compiled hot loops with non-trivial ctors are
        // rare (analysis usually rejects promotion when the ctor body has
        // any non-`this.F = p` statement).
        if (!program)
        {
            throw errors::RuntimeException(
                "No program loaded - cannot create stack object");
        }

        // MYT-208: per-frame stackObjects cap — see ObjectInstanceHelper::
        // handleNewStack for rationale. JIT-emitted NEW_STACK in a top-level
        // hot loop hits this fallback after 32 promoted allocations and reverts
        // to createObject (heap + SlotDeleter recycling) for the rest of the
        // loop, matching pre-MYT-208 throughput on workloads where the owning
        // frame can never release its stackObjects mid-loop.
        // Cap check runs BEFORE findClass so the fallback path does only one
        // hashmap lookup total.
        if (callStack.empty() ||
            callStack.back().stackObjectsCount >= CallFrame::kStackObjectsCap)
        {
            return createObject(className, args);
        }

        std::unordered_map<std::string, std::string> genericTypeBindings;
        std::string baseClassName = parseGenericTypeName(
            className, environment, genericTypeBindings);

        auto classDef = environment->findClass(baseClassName);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        auto constructor = classDef->findConstructorByTypes(args);
        if (constructor && constructor->isTrivialConstructor())
        {
            auto* raw = value::ObjectInstancePool::getInstance()
                            .acquireRaw(classDef, genericTypeBindings);
            const auto& indexed = constructor->getTrivialFieldIndexAssignments();
            raw->ensureFieldVector();
            for (const auto& [fieldIdx, paramIdx] : indexed)
            {
                if (paramIdx < args.size())
                {
                    raw->setFieldByIndex(fieldIdx, args[paramIdx]);
                }
            }
            // Lifetime owned by the calling frame's stackObjects array.
            // releaseRaw on frame teardown via CallFrame::releaseStackObjects.
            // tryPushStackObject only fails when the inline array is full,
            // which the cap check above already excluded — defensive guard
            // for the impossible case keeps lifetime accounting tight.
            if (!callStack.back().tryPushStackObject(raw))
            {
                value::ObjectInstancePool::getInstance().releaseRaw(raw);
                return createObject(className, args);
            }
            return value::makeStackObjectValue(raw);
        }

        // Default-ctor (no params, no explicit ctor): instance with no
        // initialisation — promote to STACK_OBJECT for the same workload class.
        if (!constructor && args.empty() && classDef->getConstructors().empty())
        {
            auto* raw = value::ObjectInstancePool::getInstance()
                            .acquireRaw(classDef, genericTypeBindings);
            if (!callStack.back().tryPushStackObject(raw))
            {
                value::ObjectInstancePool::getInstance().releaseRaw(raw);
                return createObject(className, args);
            }
            return value::makeStackObjectValue(raw);
        }

        // Fallback: non-trivial ctor — use the heap path. Correct, but
        // doesn't realise the STACK_OBJECT perf win for this allocation.
        return createObject(className, args);
    }

    value::Value VirtualMachine::invokeLambda(std::shared_ptr<BytecodeLambda> lambda,
                                               const std::vector<value::Value>& args)
    {
        return invokeLambda(lambda, args.data(), args.size());
    }

    value::Value VirtualMachine::invokeLambda(std::shared_ptr<BytecodeLambda> lambda,
                                               const value::Value* args,
                                               size_t argCount)
    {
        // Save current state up front so the typed catch can decorate the
        // exception against the LIVE callStack before we restore.
        size_t savedIP = instructionPointer;
        std::vector<CallFrame> savedCallStack = callStack;
        size_t savedCurrentFinallyOffset = currentFinallyOffset;

        try
        {
            if (!program)
            {
                throw errors::RuntimeException(
                    "No program loaded - cannot invoke lambda in bytecode mode without compiled bytecode");
            }

            if (!lambda)
            {
                throw errors::NullPointerException("Cannot invoke null lambda");
            }

            size_t paramCount = lambda->parameterCount;
            if (argCount != paramCount)
            {
                throw errors::RuntimeException("Lambda expects " + std::to_string(paramCount) +
                    " arguments but got " + std::to_string(argCount));
            }

            currentFinallyOffset = SIZE_MAX;
            size_t frameBase = stackManager->size();

            // Push arguments onto stack
            for (size_t i = 0; i < argCount; ++i)
            {
                push(args[i]);
            }

            // Create call frame
            CallFrame frame;
            frame.returnAddress = program->getInstructionCount();
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            // MYT-197: copy the lambda's 4-byte handle; fall back to an
            // interned display name only for lambdas that skipped handleLambda.
            if (lambda->functionName != bytecode::INVALID_FN_HANDLE) {
                frame.functionName = lambda->functionName;
            } else {
                std::string fallback = lambda->creatingClassName.empty()
                    ? "<lambda>"
                    : lambda->creatingClassName + "::<lambda>";
                frame.functionName = program->internFrameName(fallback);
            }
            frame.thisInstance = lambda->capturedThis;
            frame.definingClassName = lambda->creatingClassName;
            frame.originatingLambda = lambda;

            pushCallFrame(std::move(frame));
            stats.functionCalls++;

            // Create shared frame for this invocation, link to parent captures
            auto newSharedFrame = makePooledFrame();
            newSharedFrame->parentFrame = lambda->capturedFrame;
            if (!callStack.empty())
            {
                callStack.back().sharedFrame = newSharedFrame;
            }

            // Register parameter names in shared frame
            for (size_t i = 0; i < argCount; ++i)
            {
                if (i < lambda->parameterNames.size() && !lambda->parameterNames[i].empty())
                {
                    newSharedFrame->setLocal(lambda->parameterNames[i], i, args[i]);
                }
            }

            // Push captured variables onto stack
            if (lambda->capturedFrame)
            {
                for (size_t i = 0; i < lambda->capturedSlots.size(); ++i)
                {
                    size_t slot = lambda->capturedSlots[i];
                    value::Value capturedValue = lambda->capturedFrame->getLocal(slot);
                    push(capturedValue);
                }
            }

            // Reserve additional local variable slots if needed
            auto* lambdaMetadata = program->getFunctionMeta(lambda->functionName);
            if (lambdaMetadata)
            {
                size_t pushedSlots = argCount + lambda->capturedSlots.size();
                if (lambdaMetadata->localCount > pushedSlots)
                {
                    size_t additionalLocals = lambdaMetadata->localCount - pushedSlots;
                    for (size_t i = 0; i < additionalLocals; ++i)
                    {
                        push(std::monostate{});
                    }
                }
            }

            // Execute lambda
            instructionPointer = lambda->instructionPointer;

            // MYT-113: Async lambda -> drive via the continuation path so
            // nested awaits work and the caller gets a Promise representing
            // full body completion (used by TcpServer.onConnection callbacks).
            if (lambdaMetadata && lambdaMetadata->isAsync)
            {
                auto outerPromise = std::make_shared<value::AsyncPromiseValue>();
                driveAsyncInvocation(outerPromise, savedIP, savedCallStack,
                                     savedCurrentFinallyOffset, frameBase);
                return value::Value(
                    std::static_pointer_cast<value::PromiseValue>(outerPromise));
            }

            value::Value result = std::monostate{};
            if (controlFlowExecutor)
            {
                // Use executionCtx->program so cross-library calls fetch from the correct bytecode
                auto& lambdaCurrentProgram = executionCtx->program;
                size_t targetDepth = savedCallStack.size();
                while (callStack.size() > targetDepth)
                {
                    if (instructionPointer >= lambdaCurrentProgram->getInstructionCount())
                        break;
                    const auto& instr = lambdaCurrentProgram->getInstruction(instructionPointer);
                    try
                    {
                        executeInstruction(instr);
                    }
                    catch (errors::UserException& e)
                    {
                        auto handlerResult = exceptionHandler->handleUserException(
                            e, instructionPointer, currentFinallyOffset);
                        if (!handlerResult.handled)
                        {
                            throw;
                        }
                        // MYT-111: handler found the catch in a caller frame
                        // above our reflective boundary. Roll back its mutations
                        // so the outer VM loop's own handler runs fresh.
                        if (callStack.size() < targetDepth)
                        {
                            callStack = savedCallStack;
                            while (stackManager->size() > frameBase)
                            {
                                stackManager->pop();
                            }
                            instructionPointer = savedIP;
                            currentFinallyOffset = savedCurrentFinallyOffset;
                            throw;
                        }
                        instructionPointer = handlerResult.newInstructionPointer;
                        if (handlerResult.jumpedToFinally)
                        {
                            pendingException = std::make_unique<errors::UserException>(e);
                        }
                        continue;
                    }
                    instructionPointer++;
                }
                if (stackManager->size() > frameBase)
                    result = stackManager->pop();
                while (stackManager->size() > frameBase)
                    stackManager->pop();
            }
            else
            {
                result = interpretLoop();
            }

            // Restore state
            instructionPointer = savedIP;
            callStack = savedCallStack;
            currentFinallyOffset = savedCurrentFinallyOffset;

            return result;
        }
        catch (errors::ScriptException& e)
        {
            // MYT-46: decorate using LIVE callStack BEFORE state restore.
            utils::decorateFromCallStack(e, callStack, program);
            instructionPointer = savedIP;
            callStack = savedCallStack;
            currentFinallyOffset = savedCurrentFinallyOffset;
            throw;
        }
        catch (...)
        {
            // Restore state on error
            instructionPointer = savedIP;
            callStack = savedCallStack;
            currentFinallyOffset = savedCurrentFinallyOffset;
            throw;
        }
    }
}
