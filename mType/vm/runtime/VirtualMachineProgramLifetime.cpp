#include "VirtualMachine.hpp"

#include "../../errors/UserException.hpp"
#include "../../runtime/EventLoop.hpp"

namespace vm::runtime
{
    void VirtualMachine::prepareForProgramReplacement()
    {
        // Publish invalidation first so even a promise callback racing with
        // EventLoop cancellation observes that its captured bytecode state is
        // stale before it can touch this VM.
        programGeneration.fetch_add(1, std::memory_order_acq_rel);

        if (eventLoop)
        {
            // cancelAll() is owner-thread-only and keeps the loop reusable for
            // the next program generation.
            eventLoop->cancelAll();
        }

        currentTaskId = 0;
        savedState.reset();
        suspendedByAwait = false;
        pendingAwaitRejection.reset();
        inInteropAsyncMode = false;
        interopAwaitedPromise.reset();
        interopPendingRejection.reset();
        pendingException.reset();
        currentFinallyOffset = SIZE_MAX;
        pendingFinallyOffset = SIZE_MAX;
    }
}
