#include "VirtualMachine.hpp"
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
#include "../../value/PromiseValue.hpp"

namespace vm::runtime
{
    // Object construction & lambda invocation
    value::Value VirtualMachine::createObject(const std::string& className, const std::vector<value::Value>& args)
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

            // Find class definition
            auto classDef = environment->findClass(className);
            if (!classDef)
            {
                throw errors::ClassNotFoundException(className);
            }

            // Create object instance
            auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);

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

            // Look for constructor bytecode using type signature (matches compiler naming)
            std::string typeSignature = constructor->getTypeSignature();
            std::string qualifiedName = typeSignature.empty()
                ? className + "::<init>"
                : className + "::<init>/" + typeSignature;
            auto* ctorMetadata = program->getFunction(qualifiedName);
            if (!ctorMetadata)
            {
                throw errors::RuntimeException("Constructor '" + qualifiedName +
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
            // Set return address to end of program so interpretLoop exits after constructor returns
            frame.returnAddress = program->getInstructionCount();
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            frame.functionName = qualifiedName;
            frame.thisInstance = instance;
            frame.definingClassName = className;

            pushCallFrame(frame);
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

    value::Value VirtualMachine::invokeLambda(std::shared_ptr<BytecodeLambda> lambda,
                                               const std::vector<value::Value>& args)
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
            if (args.size() != paramCount)
            {
                throw errors::RuntimeException("Lambda expects " + std::to_string(paramCount) +
                    " arguments but got " + std::to_string(args.size()));
            }

            currentFinallyOffset = SIZE_MAX;
            size_t frameBase = stackManager->size();

            // Push arguments onto stack
            for (const auto& arg : args)
            {
                push(arg);
            }

            // Create call frame
            CallFrame frame;
            frame.returnAddress = program->getInstructionCount();
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            frame.functionName = lambda->functionName.empty() ?
                (lambda->creatingClassName.empty() ? "<lambda>" : lambda->creatingClassName + "::<lambda>") :
                lambda->functionName;
            frame.thisInstance = lambda->capturedThis;
            frame.definingClassName = lambda->creatingClassName;
            frame.originatingLambda = lambda;

            pushCallFrame(frame);
            stats.functionCalls++;

            // Create shared frame for this invocation, link to parent captures
            auto newSharedFrame = std::make_shared<SharedStackFrame>();
            newSharedFrame->parentFrame = lambda->capturedFrame;
            if (!callStack.empty())
            {
                callStack.back().sharedFrame = newSharedFrame;
            }

            // Register parameter names in shared frame
            for (size_t i = 0; i < args.size(); ++i)
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
            auto* lambdaMetadata = program->getFunction(lambda->functionName);
            if (lambdaMetadata)
            {
                size_t pushedSlots = args.size() + lambda->capturedSlots.size();
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
