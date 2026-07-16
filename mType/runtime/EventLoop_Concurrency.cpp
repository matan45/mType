#include "EventLoop.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace runtime
{
    bool EventLoopPostHandle::post(std::function<void()> callback) const
    {
        auto lockedState = state.lock();
        if (!lockedState || !callback) return false;

        std::lock_guard lock(lockedState->mutex);
        if (!lockedState->accepting ||
            lockedState->generation != generation)
        {
            return false;
        }

        lockedState->callbacks.push_back(std::move(callback));
        return true;
    }

    EventLoopPostHandle EventLoop::getPostHandle() const
    {
        auto state = postState;
        if (!state) return {};

        std::lock_guard lock(state->mutex);
        if (!state->accepting) return {};
        return EventLoopPostHandle(state, state->generation);
    }

    bool EventLoop::isOwnerThread() const noexcept
    {
        return std::this_thread::get_id() == ownerThread;
    }

    void EventLoop::requireOwnerThread(const char* operation) const
    {
        if (!isOwnerThread())
        {
            throw std::logic_error(
                std::string("EventLoop::") + operation +
                " must run on the owner thread");
        }
    }

    bool EventLoop::launchWorker(std::function<void()> worker)
    {
        requireOwnerThread("launchWorker");
        if (!worker || shuttingDown) return false;

        uint64_t generation = 0;
        {
            std::lock_guard lock(postState->mutex);
            if (!postState->accepting) return false;
            generation = postState->generation;
        }

        auto completed = std::make_shared<std::atomic<bool>>(false);
        workers.emplace_back();
        auto& record = workers.back();
        record.completed = completed;
        record.generation = generation;

        try
        {
            record.thread = std::thread(
                [job = std::move(worker), completed]() mutable {
                    try
                    {
                        job();
                    }
                    catch (...)
                    {
                        // Async helpers translate failures before returning.
                        // A foreign worker must not terminate the process.
                    }
                    completed->store(true, std::memory_order_release);
                });
        }
        catch (...)
        {
            workers.pop_back();
            throw;
        }

        return true;
    }

    void EventLoop::reapCompletedWorkers()
    {
        for (auto it = workers.begin(); it != workers.end();)
        {
            if (!it->completed ||
                !it->completed->load(std::memory_order_acquire))
            {
                ++it;
                continue;
            }

            if (it->thread.joinable()) it->thread.join();
            it = workers.erase(it);
        }
    }

    void EventLoop::joinWorkersThrough(uint64_t generation)
    {
        for (auto it = workers.begin(); it != workers.end();)
        {
            if (it->generation > generation)
            {
                ++it;
                continue;
            }

            if (it->thread.joinable()) it->thread.join();
            it = workers.erase(it);
        }
    }

    bool EventLoop::hasBackgroundWork() const
    {
        if (!workers.empty()) return true;
        std::lock_guard lock(postState->mutex);
        return !postState->callbacks.empty();
    }

    std::deque<std::function<void()>> EventLoop::takePostedCallbacks()
    {
        std::deque<std::function<void()>> callbacks;
        std::lock_guard lock(postState->mutex);
        callbacks.swap(postState->callbacks);
        return callbacks;
    }

    void EventLoop::cancelAll()
    {
        requireOwnerThread("cancelAll");
        if (shuttingDown) return;

        uint64_t cancelledGeneration = 0;
        std::deque<std::function<void()>> cancelledCallbacks;
        {
            std::lock_guard lock(postState->mutex);
            cancelledGeneration = postState->generation;
            ++postState->generation;
            cancelledCallbacks.swap(postState->callbacks);
        }

        // Advancing the generation first prevents an in-flight worker from
        // publishing after task/program captures have been released.
        joinWorkersThrough(cancelledGeneration);

        currentTask.reset();
        allTasks.clear();
        suspendedTasks.clear();
        readyQueue.clear();
        delayedTasks.clear();
        shouldStop = true;
        running = false;
    }

    void EventLoop::shutdown()
    {
        requireOwnerThread("shutdown");
        if (shuttingDown) return;

        shuttingDown = true;
        stop();

        // Owned workers are allowed to publish their plain-data completion
        // before the queue is closed. Their callbacks are then run here, on
        // the owner thread, so Promise callbacks never resume the VM on a
        // transport thread during teardown.
        joinWorkersThrough((std::numeric_limits<uint64_t>::max)());

        std::deque<std::function<void()>> completions;
        {
            std::lock_guard lock(postState->mutex);
            postState->accepting = false;
            ++postState->generation;
            completions.swap(postState->callbacks);
        }

        while (!completions.empty())
        {
            auto callback = std::move(completions.front());
            completions.pop_front();
            try
            {
                callback();
            }
            catch (...)
            {
                // Destruction cannot propagate user callback failures.
            }
        }

        currentTask.reset();
        allTasks.clear();
        suspendedTasks.clear();
        readyQueue.clear();
        delayedTasks.clear();
    }
}
