#include "VirtualMachine.hpp"
#include "utils/ExceptionHandler.hpp"
#include "../../value/ValueShim.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/ScriptException.hpp"
#include "../../errors/UserException.hpp"
#include "../../value/PromiseValue.hpp"
#include "../../value/AsyncPromiseValue.hpp"
#include "../../runtime/EventLoop.hpp"

namespace vm::runtime
{
    ::runtime::EventLoop* VirtualMachine::ensureEventLoop()
    {
        if (!eventLoop)
        {
            eventLoop = std::make_unique<::runtime::EventLoop>();
        }
        return eventLoop.get();
    }

    void VirtualMachine::executeAwait()
    {
        value::Value promiseVal = stackManager->pop();

        // Fast path: inline resolved-Promise<Int> form. The value is stored
        // directly in payload_ — no heap PromiseValue, no event-loop hop, no
        // dynamic_pointer_cast. By construction this form is only produced
        // for synchronously-resolved values, so we always take the fulfilled
        // branch.
        if (value::isPromiseInt(promiseVal))
        {
            stackManager->push(value::Value(value::asPromiseIntValue(promiseVal)));
            return;
        }

        if (!value::isPromise(promiseVal))
        {
            throw errors::RuntimeException("await can only be used on Promise values");
        }

        auto promise = value::asPromise(promiseVal);
        if (!promise)
        {
            throw errors::RuntimeException("Null promise in await expression");
        }

        if (promise->isFulfilled())
        {
            stackManager->push(promise->getValue());
            return;
        }

        if (promise->isRejected())
        {
            throw errors::UserException(
                promise->getExceptionValue(),
                promise->getExceptionTypeName()
            );
        }

        auto asyncPromise = std::dynamic_pointer_cast<value::AsyncPromiseValue>(promise);

        // MYT-113: Async-interop mode. invokeMethod/invokeStaticMethod/invokeLambda
        // for an async target run the body in this mode and drive resumption via
        // a continuation chained on the awaited promise (instead of going through
        // the event loop's task suspend/resume machinery, which can't be re-entered
        // safely from C++ interop). We hand the awaited promise back to the inner
        // loop and just flag suspension; the interop driver picks up from there.
        if (inInteropAsyncMode)
        {
            if (!asyncPromise)
            {
                throw errors::RuntimeException(
                    "Cannot await non-AsyncPromiseValue inside async interop invocation");
            }
            interopAwaitedPromise = asyncPromise;
            instructionPointer++;  // advance past AWAIT for clean resumption
            suspendedByAwait = true;
            return;
        }

        if (!eventLoop)
        {
            throw errors::RuntimeException(
                "Promise is not fulfilled and no event loop is available. "
                "Initialize an EventLoop for non-blocking async/await support.");
        }

        if (!asyncPromise)
        {
            throw errors::RuntimeException(
                "Cannot await unfulfilled Promise without AsyncPromiseValue callback support. "
                "Use AsyncPromiseValue for true async/await functionality.");
        }

        auto stackMgr = this->stackManager;
        std::weak_ptr<VirtualMachine> weakVM = weak_from_this();
        auto taskId = this->currentTaskId;

        asyncPromise->then([stackMgr, weakVM, taskId](value::Value resolvedValue)
        {
            if (auto vm = weakVM.lock())
            {
                stackMgr->push(resolvedValue);
                if (vm->eventLoop)
                    vm->eventLoop->resumeTask(taskId, resolvedValue);
            }
        });

        asyncPromise->catch_([weakVM, taskId, promise](std::string error)
        {
            if (auto vm = weakVM.lock())
            {
                vm->pendingAwaitRejection = PendingAwaitRejection{
                    promise->getExceptionValue(),
                    promise->getExceptionTypeName(),
                    error
                };
                if (vm->eventLoop)
                    vm->eventLoop->resumeTask(taskId, value::Value(std::monostate{}));
            }
        });

        instructionPointer++;
        savedState = saveState();
        eventLoop->suspendCurrentTask(promise);
        suspendedByAwait = true;
    }

    VirtualMachine::VMState VirtualMachine::saveState() const
    {
        VMState state;
        state.instructionPointer = instructionPointer;
        state.stack = stackManager->getStack(); // Get copy of entire stack
        state.callStack = callStack;
        return state;
    }

    void VirtualMachine::restoreState(const VMState& state)
    {
        instructionPointer = state.instructionPointer;
        stackManager->setStack(state.stack); // Restore entire stack
        callStack = state.callStack;
    }

    // MYT-113: Drive an async method/lambda body across nested awaits.
    //
    // Caller has already pushed the inner call frame, set instructionPointer
    // to the body's entry, and pushed any args/captures. `restoreIP` /
    // `restoreCallStack` / `restoreFinally` describe the VM state to restore
    // before this method returns (whether body completes, throws, or
    // suspends). `frameBase` marks where the outer caller's stack ends.
    //
    // On normal completion, the body's return value resolves `outerPromise`.
    // On unhandled UserException, `outerPromise` is rejected with it.
    // On AWAIT (in interop mode), the awaited promise's then/catch_ get a
    // continuation that snapshots the inner state, restores the outer state
    // visible to the resolve() caller, then re-enters this driver later.
    void VirtualMachine::driveAsyncInvocation(
        std::shared_ptr<value::AsyncPromiseValue> outerPromise,
        size_t restoreIP,
        std::vector<CallFrame> restoreCallStack,
        size_t restoreFinally,
        size_t frameBase)
    {
        bool wasInteropMode = inInteropAsyncMode;
        inInteropAsyncMode = true;

        auto& currentProgram = executionCtx->program;
        size_t targetDepth = restoreCallStack.size();

        auto restoreOuter = [&]() {
            callStack = restoreCallStack;
            while (stackManager->size() > frameBase)
                stackManager->pop();
            instructionPointer = restoreIP;
            currentFinallyOffset = restoreFinally;
            inInteropAsyncMode = wasInteropMode;
        };

        try
        {
            // A previous suspension's catch_ continuation may have left a
            // pending rejection. Surface it as the next exception.
            if (interopPendingRejection.has_value())
            {
                auto rej = std::move(interopPendingRejection.value());
                interopPendingRejection.reset();
                throw errors::UserException(rej.exceptionValue, rej.exceptionTypeName);
            }

            while (callStack.size() > targetDepth)
            {
                if (instructionPointer >= currentProgram->getInstructionCount())
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
                    if (!handlerResult.handled || callStack.size() < targetDepth)
                    {
                        throw;  // Caught below, will reject outerPromise.
                    }
                    instructionPointer = handlerResult.newInstructionPointer;
                    if (handlerResult.jumpedToFinally)
                    {
                        pendingException = std::make_unique<errors::UserException>(e);
                    }
                    continue;
                }

                // AWAIT in interop mode already advanced IP. Break out without
                // doing it again so the saved snapshot points to the next instr.
                if (suspendedByAwait)
                {
                    break;
                }
                instructionPointer++;
            }

            if (suspendedByAwait)
            {
                suspendedByAwait = false;
                auto awaited = interopAwaitedPromise;
                interopAwaitedPromise = nullptr;

                if (!awaited)
                {
                    // Defensive — shouldn't happen. Reject and bail.
                    restoreOuter();
                    outerPromise->reject("await suspended without an awaited promise");
                    return;
                }

                // Snapshot the inner state so the continuation can restore it.
                size_t innerIP = instructionPointer;
                auto innerCallStack = callStack;
                size_t innerFinally = currentFinallyOffset;
                std::vector<value::Value> innerStackTail;
                while (stackManager->size() > frameBase)
                {
                    innerStackTail.insert(innerStackTail.begin(), stackManager->pop());
                }

                // Restore the caller's state so resolve() can keep going cleanly.
                restoreOuter();

                std::weak_ptr<VirtualMachine> weakSelf = weak_from_this();

                auto resumeContinuation =
                    [weakSelf, outerPromise, frameBase, innerIP, innerCallStack,
                     innerFinally, innerStackTail](value::Value resolvedValue) mutable
                {
                    auto self = weakSelf.lock();
                    if (!self) return;

                    // Snapshot the VM's state at continuation entry, restore the
                    // inner state, push the awaited result, drive the body. On
                    // exit (suspend / complete / throw) the driver restores the
                    // snapshot we captured here so the post-callback resumes
                    // its tick cleanly.
                    size_t snapIP = self->instructionPointer;
                    auto snapCallStack = self->callStack;
                    size_t snapFinally = self->currentFinallyOffset;

                    self->callStack = innerCallStack;
                    self->instructionPointer = innerIP;
                    self->currentFinallyOffset = innerFinally;
                    for (auto& v : innerStackTail) self->stackManager->push(v);
                    self->stackManager->push(resolvedValue);

                    self->driveAsyncInvocation(outerPromise, snapIP, snapCallStack,
                                               snapFinally, frameBase);
                };

                auto rejectContinuation =
                    [weakSelf, outerPromise, frameBase, innerIP, innerCallStack,
                     innerFinally, innerStackTail, awaited](std::string error) mutable
                {
                    auto self = weakSelf.lock();
                    if (!self) return;

                    size_t snapIP = self->instructionPointer;
                    auto snapCallStack = self->callStack;
                    size_t snapFinally = self->currentFinallyOffset;

                    self->callStack = innerCallStack;
                    self->instructionPointer = innerIP;
                    self->currentFinallyOffset = innerFinally;
                    for (auto& v : innerStackTail) self->stackManager->push(v);

                    self->interopPendingRejection = PendingAwaitRejection{
                        awaited->getExceptionValue(),
                        awaited->getExceptionTypeName(),
                        error
                    };

                    self->driveAsyncInvocation(outerPromise, snapIP, snapCallStack,
                                               snapFinally, frameBase);
                };

                awaited->then(resumeContinuation);
                awaited->catch_(rejectContinuation);
                return;  // outerPromise settles when the continuation runs.
            }

            // Body completed.
            value::Value finalResult = std::monostate{};
            if (stackManager->size() > frameBase)
            {
                finalResult = stackManager->pop();
            }
            restoreOuter();
            outerPromise->resolve(finalResult);
        }
        catch (errors::UserException& e)
        {
            restoreOuter();
            outerPromise->rejectWithException(
                e.getExceptionValue(), e.getExceptionTypeName(), e.what());
        }
        catch (errors::ScriptException& e)
        {
            restoreOuter();
            outerPromise->reject(e.what());
        }
        catch (const std::exception& e)
        {
            restoreOuter();
            outerPromise->reject(e.what());
        }
    }
}
