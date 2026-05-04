#include "VirtualMachine.hpp"
#include <cstdint>
#include "executors/ControlFlowExecutor.hpp"
#include "utils/InteropExceptionDecorator.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/ClassNotFoundException.hpp"
#include "../../errors/MethodNotFoundException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../../errors/ScriptException.hpp"
#include "../../errors/UserException.hpp"
#include "utils/ExceptionHandler.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../value/AsyncPromiseValue.hpp"
#include "../../value/PromiseValue.hpp"

namespace vm::runtime
{
    value::Value VirtualMachine::invokeMethod(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                              const std::string& methodName,
                                              std::span<const value::Value> args)
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
                    "No program loaded - cannot invoke method in bytecode mode without compiled bytecode");
            }

            if (!instance)
            {
                throw errors::NullPointerException("Cannot call method '" + methodName + "' on null object");
            }

            auto classDef = instance->getClassDefinition();
            size_t argCount = args.size();

            // Find method in hierarchy
            auto method = classDef->findInstanceMethodInHierarchy(methodName, argCount);
            if (!method)
            {
                // MYT-46: typed throw so the converter routes this to MT-E1007
                // (NameMethodNotFound) instead of the catchall MT-E5005.
                throw errors::MethodNotFoundException(methodName, classDef->getName());
            }

            // Find which class defines this method and get the type signature
            std::string definingClassName = classDef->getName();
            std::string typeSignature;
            auto currentClass = classDef;
            while (currentClass)
            {
                auto localMethod = currentClass->findInstanceMethod(methodName, argCount);
                if (localMethod)
                {
                    definingClassName = currentClass->getName();
                    typeSignature = localMethod->getTypeSignature();
                    break;
                }
                currentClass = currentClass->getParentClass();
            }

            // Look for method bytecode using type signature (matches compiler naming)
            std::string qualifiedName = typeSignature.empty()
                ? definingClassName + "::" + methodName
                : definingClassName + "::" + methodName + "/" + typeSignature;
            auto* funcMetadata = program->getFunction(qualifiedName);
            if (!funcMetadata)
            {
                throw errors::RuntimeException("Method '" + qualifiedName +
                    "' has no bytecode. Bytecode compilation is required for VM execution.");
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
            // Set return address to end of program so interpretLoop exits after method returns
            frame.returnAddress = program->getInstructionCount();
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            frame.functionName = program->internFrameName(qualifiedName);
            frame.thisInstance = instance;
            frame.definingClassName = definingClassName;

            pushCallFrame(std::move(frame));
            stats.functionCalls++;

            // Execute method (direct loop with UserException handling for try/catch support)
            instructionPointer = funcMetadata->startOffset;

            // MYT-113: For async target methods, drive via the continuation-
            // based interop async path so nested awaits work and the caller
            // gets a Promise representing full body completion.
            if (method->getIsAsync())
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
                auto& currentProgram = executionCtx->program;
                size_t targetDepth = savedCallStack.size();
                while (callStack.size() > targetDepth)
                {
                    if (instructionPointer >= currentProgram->getInstructionCount())
                        break;
                    if (suspendedByAwait)
                        break;
                    const auto& instr = currentProgram->getInstruction(instructionPointer);
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
                        // above our reflective boundary and has already unwound
                        // callStack + pushed the exception value. That state
                        // only makes sense to the *outer* VM loop; we can't
                        // resume from it here (IP is in a popped frame). Roll
                        // back the handler's mutations so the outer loop sees
                        // a pristine state and invokes its own handler fresh.
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
                if (!suspendedByAwait && stackManager->size() > frameBase)
                    result = stackManager->pop();
                if (!suspendedByAwait)
                {
                    while (stackManager->size() > frameBase)
                        stackManager->pop();
                }
            }
            else
            {
                result = interpretLoop();
            }

            // If suspended by await (sync method that somehow awaited — kept
            // for backwards compatibility), don't restore state — EventLoop
            // will resume.
            if (suspendedByAwait)
            {
                suspendedByAwait = false;
                return result;
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

    value::Value VirtualMachine::invokeStaticMethod(const std::string& className,
                                                    const std::string& methodName,
                                                    std::span<const value::Value> args)
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
                    "No program loaded - cannot invoke static method in bytecode mode without compiled bytecode");
            }

            auto classDef = environment->findClass(className);
            if (!classDef)
            {
                throw errors::ClassNotFoundException(className);
            }

            size_t argCount = args.size();
            auto method = classDef->findStaticMethod(methodName, argCount);
            if (!method)
            {
                // MYT-46: typed throw so the converter routes this to MT-E1007
                // (NameMethodNotFound) instead of the catchall MT-E5005.
                throw errors::MethodNotFoundException(methodName, className);
            }

            // Look for method bytecode using type signature (matches compiler naming)
            std::string typeSignature = method->getTypeSignature();
            std::string qualifiedName = typeSignature.empty()
                ? className + "::" + methodName
                : className + "::" + methodName + "/" + typeSignature;
            // For static methods, append $static suffix
            if (method->isStatic()) {
                qualifiedName += "$static";
            }
            auto* funcMetadata = program->getFunction(qualifiedName);

            // Fallback: the method's type signature (e.g. "array") may differ from the
            // compiler's function name (e.g. "string[]"). Scan for a matching function.
            if (!funcMetadata)
            {
                std::string prefix = className + "::" + methodName;
                std::string suffix = method->isStatic() ? "$static" : "";
                for (const auto& [fname, fmeta] : program->getFunctions())
                {
                    if (fname.rfind(prefix, 0) == 0 &&
                        (!suffix.empty() ? fname.find(suffix) != std::string::npos : true) &&
                        fname != prefix + suffix)
                    {
                        funcMetadata = program->getFunction(fname);
                        qualifiedName = fname;  // Update to match actual function name
                        break;
                    }
                }
            }

            if (!funcMetadata)
            {
                throw errors::RuntimeException("Static method '" + qualifiedName +
                    "' has no bytecode. Bytecode compilation is required for VM execution.");
            }

            // Reset finally offset for new function call
            currentFinallyOffset = SIZE_MAX;

            // Push arguments onto stack
            size_t frameBase = stackManager->size();
            for (const auto& arg : args)
            {
                push(arg);
            }

            // Create call frame
            CallFrame frame;
            // Set return address to end of program so interpretLoop exits after method returns
            frame.returnAddress = program->getInstructionCount();
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            frame.functionName = program->internFrameName(qualifiedName);
            frame.thisInstance = nullptr; // Static methods have no 'this'
            frame.definingClassName = className;

            pushCallFrame(std::move(frame));
            stats.functionCalls++;

            // Execute method (direct loop with UserException handling for try/catch support)
            instructionPointer = funcMetadata->startOffset;

            // MYT-113: Async target -> drive via the continuation path.
            if (method->getIsAsync())
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
                auto& currentProgram = executionCtx->program;
                size_t targetDepth = savedCallStack.size();
                while (callStack.size() > targetDepth)
                {
                    if (instructionPointer >= currentProgram->getInstructionCount())
                        break;
                    if (suspendedByAwait)
                        break;
                    const auto& instr = currentProgram->getInstruction(instructionPointer);
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
                            throw;  // Re-throw if no handler found
                        }
                        // MYT-111: see invokeMethod for the detailed comment.
                        // Roll back handler mutations so the outer VM loop's
                        // own handler runs fresh against our entry state.
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
                if (!suspendedByAwait && stackManager->size() > frameBase)
                    result = stackManager->pop();
                if (!suspendedByAwait)
                {
                    while (stackManager->size() > frameBase)
                        stackManager->pop();
                }
            }
            else
            {
                result = interpretLoop();
            }

            // If suspended by await, don't restore state — EventLoop will resume
            if (suspendedByAwait)
            {
                suspendedByAwait = false;
                return result;
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
