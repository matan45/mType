#include "EventLoop.hpp"
#include "../value/PromiseValue.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include <algorithm>

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

        // If task has a resume callback, invoke it
        if (task->resumeCallback) {
            task->resumeCallback(resolvedValue);
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

        // Safety timeout to prevent infinite loops during debugging
        auto startTime = std::chrono::steady_clock::now();
        const int MAX_ITERATIONS = 10000;
        int iterations = 0;

        while (!shouldStop && tick()) {
            iterations++;

            // Check for timeout (10 seconds) or max iterations
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - startTime
            ).count();

            if (elapsed > 10 || iterations > MAX_ITERATIONS) {
                break;
            }
        }

        running = false;
    }

    bool EventLoop::tick() {
        // Process callbacks from background threads
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            while (!pendingCallbacks.empty()) {
                auto callback = pendingCallbacks.front();
                pendingCallbacks.pop_front();
                callback();
            }
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

        return true; // More work might be available
    }

    void EventLoop::stop() {
        shouldStop = true;
    }

    size_t EventLoop::getPendingTaskCount() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(queueMutex));
        return readyQueue.size() + suspendedTasks.size() + delayedTasks.size();
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

            // Remove from all tasks
            std::lock_guard<std::mutex> lock(queueMutex);
            allTasks.erase(task->taskId);
        }
        catch (const std::exception& e) {
            task->state = TaskState::FAILED;
            task->errorMessage = e.what();

            // Reject the promise
            if (task->resultPromise) {
                task->resultPromise->reject(e.what());
            }

            // Remove from all tasks
            std::lock_guard<std::mutex> lock(queueMutex);
            allTasks.erase(task->taskId);
        }

        currentTask = nullptr;
    }

    void EventLoop::checkDelayedTasks() {
        // This is now handled by moveReadyDelayedTasks()
    }

    void EventLoop::checkCompletedPromises() {
        std::lock_guard<std::mutex> lock(queueMutex);

        // Check all suspended tasks
        std::vector<size_t> toResume;

        for (const auto& [taskId, task] : suspendedTasks) {
            if (task->waitingOn && task->waitingOn->isFulfilled()) {
                toResume.push_back(taskId);
            }
        }

        // Resume tasks whose promises are fulfilled
        for (size_t taskId : toResume) {
            auto task = suspendedTasks[taskId];
            value::Value resolvedValue = task->waitingOn->getValue();

            task->state = TaskState::PENDING;
            task->waitingOn = nullptr;

            readyQueue.push_back(task);
            suspendedTasks.erase(taskId);
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

} // namespace runtime
