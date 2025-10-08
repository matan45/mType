#pragma once

#include "../value/ValueType.hpp"
#include <memory>
#include <deque>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <mutex>

namespace vm::runtime {
    // Forward declarations
    class VirtualMachine;
    class ExecutionContext;
}

namespace value {
    class PromiseValue;
}

namespace runtime {

    /**
     * @brief State of an asynchronous task
     */
    enum class TaskState {
        PENDING,    // Not yet started
        RUNNING,    // Currently executing
        SUSPENDED,  // Waiting on promise or I/O
        COMPLETED,  // Finished successfully
        FAILED      // Encountered error
    };

    /**
     * @brief Saved execution state for task suspension/resumption
     */
    struct ExecutionSnapshot {
        size_t instructionPointer;
        std::vector<value::Value> stack;
        std::unordered_map<std::string, value::Value> locals;

        // For nested function calls
        std::vector<size_t> callStack;
    };

    /**
     * @brief Represents an asynchronous task in the event loop
     */
    struct Task {
        size_t taskId;
        TaskState state;

        // Execution state
        std::shared_ptr<ExecutionSnapshot> snapshot;

        // The function to execute (for initial scheduling)
        std::function<value::Value()> function;

        // Promise this task will fulfill
        std::shared_ptr<value::PromiseValue> resultPromise;

        // Promise this task is waiting on (if suspended)
        std::shared_ptr<value::PromiseValue> waitingOn;

        // Callback to resume execution
        std::function<void(value::Value)> resumeCallback;

        // Priority for scheduling (higher = more urgent)
        int priority;

        // When this task was scheduled
        std::chrono::steady_clock::time_point scheduledAt;

        // Error information if failed
        std::string errorMessage;

        Task(size_t id)
            : taskId(id)
            , state(TaskState::PENDING)
            , priority(0)
            , scheduledAt(std::chrono::steady_clock::now())
        {}
    };

    /**
     * @brief Delayed task (for setTimeout, etc.)
     */
    struct DelayedTask {
        std::shared_ptr<Task> task;
        std::chrono::steady_clock::time_point executeAt;

        bool isReady() const {
            return std::chrono::steady_clock::now() >= executeAt;
        }
    };

    /**
     * @brief Event Loop for cooperative multitasking
     *
     * Implements a single-threaded event loop that schedules and executes
     * async tasks cooperatively. Tasks can suspend themselves when waiting
     * for promises and resume when promises are fulfilled.
     *
     * Design:
     * - Single-threaded (no thread safety overhead)
     * - Cooperative (tasks must explicitly yield)
     * - Priority-based scheduling
     * - Supports delayed execution (setTimeout)
     *
     * Usage:
     * @code
     * EventLoop loop;
     * loop.scheduleTask([]() { return asyncFunction(); });
     * loop.run();  // Run until all tasks complete
     * @endcode
     */
    class EventLoop {
    private:
        // Task queues
        std::deque<std::shared_ptr<Task>> readyQueue;           // Tasks ready to run
        std::unordered_map<size_t, std::shared_ptr<Task>> suspendedTasks;  // Waiting tasks
        std::unordered_map<size_t, std::shared_ptr<Task>> allTasks;        // All tasks
        std::deque<DelayedTask> delayedTasks;                   // Scheduled for future

        // State
        size_t nextTaskId;
        std::shared_ptr<Task> currentTask;
        bool running;
        bool shouldStop;

        // Thread safety for cross-thread promise resolution
        std::mutex queueMutex;

    public:
        EventLoop();
        ~EventLoop();

        /**
         * @brief Schedule a new async task for execution
         * @param asyncFunction The async function to execute
         * @param priority Task priority (higher = more urgent)
         * @return Task ID for tracking
         */
        size_t scheduleTask(
            std::function<value::Value()> asyncFunction,
            int priority = 0
        );

        /**
         * @brief Schedule a delayed task (setTimeout)
         * @param asyncFunction The function to execute
         * @param delayMs Delay in milliseconds
         * @return Task ID
         */
        size_t scheduleDelayedTask(
            std::function<value::Value()> asyncFunction,
            int delayMs
        );

        /**
         * @brief Suspend current task and wait for promise
         * @param promise The promise to wait for
         */
        void suspendCurrentTask(std::shared_ptr<value::PromiseValue> promise);

        /**
         * @brief Resume a suspended task when promise resolves
         * @param taskId ID of task to resume
         * @param resolvedValue Value the promise resolved to
         */
        void resumeTask(size_t taskId, value::Value resolvedValue);

        /**
         * @brief Cancel a task
         * @param taskId ID of task to cancel
         */
        void cancelTask(size_t taskId);

        /**
         * @brief Run the event loop until all tasks complete
         */
        void run();

        /**
         * @brief Execute one iteration of the event loop
         * @return true if there are more tasks to process
         */
        bool tick();

        /**
         * @brief Stop the event loop gracefully
         */
        void stop();

        /**
         * @brief Check if event loop is currently running
         */
        bool isRunning() const { return running; }

        /**
         * @brief Get current task (or nullptr if none executing)
         */
        std::shared_ptr<Task> getCurrentTask() const { return currentTask; }

        /**
         * @brief Get number of pending tasks
         */
        size_t getPendingTaskCount() const;

        /**
         * @brief Post a callback to run on next event loop iteration
         * Thread-safe - can be called from background threads
         */
        void post(std::function<void()> callback);

    private:
        void executeTask(std::shared_ptr<Task> task);
        void checkDelayedTasks();
        void checkCompletedPromises();
        void moveReadyDelayedTasks();
        std::shared_ptr<Task> selectNextTask();

        // Queue of callbacks from background threads
        std::deque<std::function<void()>> pendingCallbacks;
    };

    /**
     * @brief Global event loop instance
     *
     * For simplicity, we use a global event loop. In a more complex implementation,
     * you might have multiple event loops or pass the event loop as a parameter.
     */
    extern EventLoop* globalEventLoop;

    /**
     * @brief Initialize global event loop
     */
    void initializeEventLoop();

    /**
     * @brief Cleanup global event loop
     */
    void shutdownEventLoop();

} // namespace runtime
