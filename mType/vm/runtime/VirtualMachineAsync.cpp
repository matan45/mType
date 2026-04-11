#include "VirtualMachine.hpp"
#include "../../errors/RuntimeException.hpp"
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
        if (!std::holds_alternative<std::shared_ptr<value::PromiseValue>>(promiseVal))
        {
            throw errors::RuntimeException("await can only be used on Promise values");
        }

        auto promise = std::get<std::shared_ptr<value::PromiseValue>>(promiseVal);
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

        if (!eventLoop)
        {
            throw errors::RuntimeException(
                "Promise is not fulfilled and no event loop is available. "
                "Initialize an EventLoop for non-blocking async/await support.");
        }

        auto asyncPromise = std::dynamic_pointer_cast<value::AsyncPromiseValue>(promise);
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
}
