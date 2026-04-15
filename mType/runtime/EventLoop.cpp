#include "EventLoop.hpp"
#include "../value/PromiseValue.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include <algorithm>
#include <iostream>

namespace runtime {

    EventLoop::EventLoop()
        : nextTaskId(1)
        , currentTask(nullptr)
        , running(false)
        , shouldStop(false)
        , agingInterval(100)  // Default: 1 priority point per 100ms
    {
    }

    EventLoop::~EventLoop() {
        stop();

        // Clean up all remaining tasks to prevent memory leaks
        std::lock_guard<std::mutex> lock(queueMutex);
        allTasks.clear();
        suspendedTasks.clear();
        readyQueue.clear();
        delayedTasks.clear();
        pendingCallbacks.clear();
    }

    size_t EventLoop::scheduleTask(
        std::function<value::Value()> asyncFunction)
    {
        std::lock_guard<std::mutex> lock(queueMutex);

        size_t taskId = nextTaskId++;
        auto task = std::make_shared<Task>(taskId);
        task->function = asyncFunction;
        task->priority = 0;  // Default priority managed by EventLoop
        task->state = TaskState::PENDING;

        // Create a promise for this task's result
        task->resultPromise = std::make_shared<value::PromiseValue>();

        allTasks[taskId] = task;
        readyQueue.push_back(task);

        return taskId;
    }

    size_t EventLoop::scheduleDelayedTask(
        std::function<value::Value()> asyncFunction,
        int delayMs,
        int priority)
    {
        std::lock_guard<std::mutex> lock(queueMutex);

        size_t taskId = nextTaskId++;
        auto task = std::make_shared<Task>(taskId);
        task->function = asyncFunction;
        task->priority = priority;
        task->state = TaskState::PENDING;
        task->resultPromise = std::make_shared<value::PromiseValue>();

        allTasks[taskId] = task;

        // Schedule for future execution
        DelayedTask delayed;
        delayed.task = task;
        delayed.executeAt = std::chrono::steady_clock::now() +
                           std::chrono::milliseconds(delayMs);

        delayedTasks.push_back(delayed);

        return taskId;
    }

    void EventLoop::suspendCurrentTask(std::shared_ptr<value::PromiseValue> promise) {
        if (!currentTask) {
            throw std::runtime_error("Cannot suspend - no current task");
        }

        std::lock_guard<std::mutex> lock(queueMutex);

        currentTask->state = TaskState::SUSPENDED;
        currentTask->waitingOn = promise;

        // Move to suspended tasks map
        suspendedTasks[currentTask->taskId] = currentTask;
    }

    void EventLoop::resumeTask(size_t taskId, value::Value resolvedValue) {
        std::function<void(value::Value)> callback;

        {
            std::lock_guard<std::mutex> lock(queueMutex);

            auto it = suspendedTasks.find(taskId);
            if (it == suspendedTasks.end()) {
                return;
            }

            auto task = it->second;
            task->state = TaskState::PENDING;
            task->waitingOn = nullptr;

            // Add back to ready queue
            readyQueue.push_back(task);
            suspendedTasks.erase(it);

            // Capture callback to execute outside the lock
            callback = task->resumeCallback;
        }

        // Execute callback outside queueMutex to prevent lock inversion
        // (resolve path holds callbackMutex -> calls resumeTask -> would acquire queueMutex)
        if (callback) {
            callback(resolvedValue);
        }
    }

    void EventLoop::cancelTask(size_t taskId) {
        std::lock_guard<std::mutex> lock(queueMutex);

        auto it = allTasks.find(taskId);
        if (it != allTasks.end()) {
            auto task = it->second;
            task->state = TaskState::FAILED;
            task->errorMessage = "Task cancelled";

            // Remove from queues
            suspendedTasks.erase(taskId);
            readyQueue.erase(
                std::remove_if(readyQueue.begin(), readyQueue.end(),
                    [taskId](const auto& t) { return t->taskId == taskId; }),
                readyQueue.end()
            );

            allTasks.erase(it);
        }
    }

    void EventLoop::setTaskVM(size_t taskId, std::shared_ptr<vm::runtime::VirtualMachine> vmPtr) {
        std::lock_guard<std::mutex> lock(queueMutex);

        auto it = allTasks.find(taskId);
        if (it != allTasks.end()) {
            it->second->vm = vmPtr;  // Assigns shared_ptr to weak_ptr
        }
    }

    void EventLoop::run() {
        running = true;
        shouldStop = false;

        // Safety limits to prevent runaway event loops.
        // These are intentionally conservative for the current MVP scope.
        auto startTime = std::chrono::steady_clock::now();
        constexpr int MAX_ITERATIONS = 10000;
        constexpr int MAX_SECONDS = 10;
        int iterations = 0;

        while (!shouldStop && tick()) {
            iterations++;

            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - startTime
            ).count();

            if (elapsed > MAX_SECONDS || iterations > MAX_ITERATIONS) {
                break;
            }
        }

        running = false;
    }

    bool EventLoop::tick() {
        // Process callbacks from background threads. Drain the queue under
        // the lock, then execute callbacks WITHOUT the lock held — callbacks
        // may legitimately call post() / scheduleTask() / resumeTask() (e.g.
        // a promise resolution that resumes a suspended task), all of which
        // lock queueMutex themselves. Holding the lock across callback
        // execution would re-lock it on the same thread and trigger
        // resource_deadlock_would_occur.
        std::deque<std::function<void()>> drained;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            drained.swap(pendingCallbacks);
        }
        while (!drained.empty()) {
            auto callback = std::move(drained.front());
            drained.pop_front();
            callback();
        }

        // Move delayed tasks that are ready
        moveReadyDelayedTasks();

        // Check if any suspended tasks can be resumed
        checkCompletedPromises();

        // Select next task to execute
        auto task = selectNextTask();

        if (!task) {
            // No tasks ready - check if we have suspended or delayed tasks
            std::lock_guard<std::mutex> lock(queueMutex);
            bool hasWork = !suspendedTasks.empty() || !delayedTasks.empty();

            if (hasWork) {
                // Sleep briefly to avoid busy-waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                return true;
            }

            return false; // No more work
        }

        // Execute the task
        executeTask(task);

        // Clean up completed/failed tasks that are no longer in any queue
        cleanupCompletedTasks();

        return true; // More work might be available
    }

    void EventLoop::stop() {
        shouldStop = true;
    }

    void EventLoop::post(std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(queueMutex);
        pendingCallbacks.push_back(callback);
    }

    void EventLoop::executeTask(std::shared_ptr<Task> task) {
        currentTask = task;
        task->state = TaskState::RUNNING;

        // Set task ID in VM if available (safely check if VM still exists)
        if (auto vmPtr = task->vm.lock()) {
            vmPtr->setCurrentTaskId(task->taskId);
        }

        try {
            // Execute the task's function
            value::Value result = task->function();

            // Check if task was suspended during execution
            // If suspended, the task state was changed by suspendCurrentTask()
            if (task->state == TaskState::SUSPENDED) {
                // Don't mark as completed, task is in suspendedTasks map
                return;
            }

            // Task completed successfully
            task->state = TaskState::COMPLETED;

            // Resolve the task's result promise
            if (task->resultPromise) {
                task->resultPromise->resolve(result);
            }

            // Keep task in allTasks so caller can check its status
            // Caller is responsible for cleanup
        }
        catch (const std::exception& e) {
            task->state = TaskState::FAILED;
            task->errorMessage = e.what();

            // Reject the promise
            if (task->resultPromise) {
                task->resultPromise->reject(e.what());
            }

            // Keep task in allTasks so caller can check its status and error message
            // Caller is responsible for cleanup
        }

        currentTask = nullptr;
    }

    void EventLoop::checkCompletedPromises() {
        // Collect rejections under the lock, then invoke promise->reject() after
        // release. reject() synchronously runs .then/.catch handlers, which may
        // call post() / scheduleTask() and re-lock queueMutex on this thread.
        std::vector<std::pair<std::shared_ptr<Task>, std::string>> rejectedTasks;

        {
            std::lock_guard<std::mutex> lock(queueMutex);

            std::vector<size_t> toResume;
            for (const auto& [taskId, task] : suspendedTasks) {
                if (task->waitingOn && (task->waitingOn->isFulfilled() || task->waitingOn->isRejected())) {
                    toResume.push_back(taskId);
                }
            }

            for (size_t taskId : toResume) {
                auto task = suspendedTasks[taskId];

                if (task->waitingOn->isRejected()) {
                    // Mark task as failed — the VM's pendingAwaitRejection mechanism
                    // handles proper error propagation for tasks resumed via callbacks.
                    // This path catches tasks waiting on plain PromiseValue (no callbacks).
                    task->state = TaskState::FAILED;
                    task->errorMessage = task->waitingOn->getError();

                    if (task->resultPromise) {
                        rejectedTasks.emplace_back(task, task->errorMessage);
                    }
                } else {
                    task->state = TaskState::PENDING;
                    readyQueue.push_back(task);
                }

                task->waitingOn = nullptr;
                suspendedTasks.erase(taskId);
            }
        }

        for (auto& [task, err] : rejectedTasks) {
            task->resultPromise->reject(err);
        }
    }

    void EventLoop::moveReadyDelayedTasks() {
        std::lock_guard<std::mutex> lock(queueMutex);

        auto now = std::chrono::steady_clock::now();

        // Find all delayed tasks that are ready
        auto it = delayedTasks.begin();
        while (it != delayedTasks.end()) {
            if (it->executeAt <= now) {
                // Move to ready queue
                readyQueue.push_back(it->task);
                it = delayedTasks.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::shared_ptr<Task> EventLoop::selectNextTask() {
        std::lock_guard<std::mutex> lock(queueMutex);

        if (readyQueue.empty()) {
            return nullptr;
        }

        // Priority-based scheduling with aging to prevent starvation
        // Effective Priority = Base Priority + Age Bonus
        // Age Bonus = milliseconds waiting / agingInterval
        // This ensures older tasks gradually gain priority

        auto now = std::chrono::steady_clock::now();
        auto highestPriorityIt = readyQueue.begin();

        // Calculate effective priority for first task
        auto waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - (*highestPriorityIt)->scheduledAt
        ).count();
        int ageBonus = static_cast<int>(waitTime / agingInterval);
        int highestEffectivePriority = (*highestPriorityIt)->priority + ageBonus;

        // Find task with highest effective priority
        for (auto it = readyQueue.begin(); it != readyQueue.end(); ++it) {
            waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - (*it)->scheduledAt
            ).count();
            ageBonus = static_cast<int>(waitTime / agingInterval);
            int effectivePriority = (*it)->priority + ageBonus;

            if (effectivePriority > highestEffectivePriority) {
                highestEffectivePriority = effectivePriority;
                highestPriorityIt = it;
            }
        }

        auto task = *highestPriorityIt;
        readyQueue.erase(highestPriorityIt);

        return task;
    }

    std::shared_ptr<Task> EventLoop::getTask(size_t taskId) const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(queueMutex));
        auto it = allTasks.find(taskId);
        if (it != allTasks.end()) {
            return it->second;
        }
        return nullptr;
    }

    void EventLoop::cleanupCompletedTasks() {
        std::lock_guard<std::mutex> lock(queueMutex);

        std::vector<size_t> toRemove;
        for (const auto& [taskId, task] : allTasks) {
            if (task->state == TaskState::COMPLETED || task->state == TaskState::FAILED) {
                // Only remove if not in any active queue
                if (suspendedTasks.find(taskId) == suspendedTasks.end()) {
                    toRemove.push_back(taskId);
                }
            }
        }

        for (size_t taskId : toRemove) {
            allTasks.erase(taskId);
        }
    }

} // namespace runtime
