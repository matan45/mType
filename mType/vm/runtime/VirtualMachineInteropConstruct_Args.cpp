#include "VirtualMachine.hpp"
#include <cstddef>
#include "context/SharedStackFramePool.hpp"
#include "executors/ControlFlowExecutor.hpp"
#include "utils/ExceptionHandler.hpp"
#include "utils/InteropExceptionDecorator.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/ScriptException.hpp"
#include "../../errors/UserException.hpp"
#include "../../value/AsyncPromiseValue.hpp"
#include "../../value/PromiseValue.hpp"

namespace vm::runtime
{
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

            for (size_t i = 0; i < argCount; ++i)
            {
                push(args[i]);
            }

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

            auto newSharedFrame = makePooledFrame();
            newSharedFrame->parentFrame = lambda->capturedFrame;
            if (!callStack.empty())
            {
                callStack.back().sharedFrame = newSharedFrame;
            }

            for (size_t i = 0; i < argCount; ++i)
            {
                if (i < lambda->parameterNames.size() && !lambda->parameterNames[i].empty())
                {
                    newSharedFrame->setLocal(lambda->parameterNames[i], i, args[i]);
                }
            }

            if (lambda->capturedFrame)
            {
                for (size_t i = 0; i < lambda->capturedSlots.size(); ++i)
                {
                    size_t slot = lambda->capturedSlots[i];
                    value::Value capturedValue = lambda->capturedFrame->getLocal(slot);
                    push(capturedValue);
                }
            }

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

            instructionPointer = lambda->instructionPointer;

            // MYT-113: Async lambda → drive via the continuation path so
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
                // executionCtx->program so cross-library calls fetch the correct bytecode.
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
            instructionPointer = savedIP;
            callStack = savedCallStack;
            currentFinallyOffset = savedCurrentFinallyOffset;
            throw;
        }
    }
}
