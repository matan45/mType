#pragma once

#include "../value/ValueType.hpp"
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <memory>
#include <deque>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

namespace vm::runtime {
    // Forward declarations
    class VirtualMachine;
    class ExecutionContext;
}

namespace value {
    class PromiseValue;
}

namespace runtime {

    namespace detail {
        struct EventLoopPostState {
            mutable std::mutex mutex;
            std::deque<std::function<void()>> callbacks;
            uint64_t generation = 1;
            bool accepting = true;
        };
    }

    /**
     * Thread-safe, lifetime-safe producer endpoint for EventLoop::post().
     * A handle is bound to the event-loop generation in which it was created;
     * cancelAll() invalidates old handles without leaving a raw EventLoop* on
     * background threads.
     */
    class EventLoopPostHandle {
    public:
        EventLoopPostHandle() = default;

        bool post(std::function<void()> callback) const;
        explicit operator bool() const noexcept { return !state.expired(); }

    private:
        friend class EventLoop;

        EventLoopPostHandle(
            std::weak_ptr<detail::EventLoopPostState> state,
            uint64_t generation) noexcept
            : state(std::move(state)), generation(generation)
        {
        }

        std::weak_ptr<detail::EventLoopPostState> state;
        uint64_t generation = 0;
    };

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

        // VM reference (optional, for setting task ID before execution)
        // Uses weak_ptr to prevent dangling pointer crashes if VM is destroyed
        std::weak_ptr<vm::runtime::VirtualMachine> vm;

        // Priority for scheduling (higher = more urgent)
        int priority;

        // When this task was scheduled
        std::chrono::steady_clock::time_point scheduledAt;

        // Error information if failed
        std::string errorMessage;

        Task(size_t id)
            : taskId(id)
            , state(TaskState::PENDING)
            , vm()  // Default-constructed weak_ptr (empty)
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
     * - VM task state is single-owner-thread; producer posting is synchronized
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

        // Priority aging configuration
        // Age bonus = (wait_time_ms / agingInterval)
        // Default: 1 priority point per 100ms of waiting
        int agingInterval;

        struct WorkerRecord {
            std::thread thread;
            std::shared_ptr<std::atomic<bool>> completed;
            uint64_t generation = 0;
        };

        std::shared_ptr<detail::EventLoopPostState> postState;
        std::thread::id ownerThread;
        std::vector<WorkerRecord> workers;
        bool shuttingDown;

    public:
        EventLoop();
        ~EventLoop();

        /**
         * @brief Schedule a new async task for execution
         * @param asyncFunction The async function to execute
         * @return Task ID for tracking
         */
        size_t scheduleTask(
            std::function<value::Value()> asyncFunction
        );

        /**
         * @brief Schedule a delayed task (setTimeout)
         * @param asyncFunction The function to execute
         * @param delayMs Delay in milliseconds
         * @return Task ID
         */
        size_t scheduleDelayedTask(
            std::function<value::Value()> asyncFunction,
            int delayMs,
            int priority = 0
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
         * @brief Set VM reference for a task (for automatic task ID setting)
         * @param taskId ID of the task
         * @param vmPtr Shared pointer to the VirtualMachine
         */
        void setTaskVM(size_t taskId, std::shared_ptr<vm::runtime::VirtualMachine> vmPtr);

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
         * Invalidate all work belonging to the current program generation.
         * Must be called on the event-loop owner thread. Old post handles stop
         * accepting callbacks; owned workers are joined before task captures
         * are released. The EventLoop remains reusable afterward.
         */
        void cancelAll();

        /**
         * Terminal shutdown. Joins owned workers and settles completions that
         * they already posted on the owner thread before closing the queue.
         */
        void shutdown();

        bool isOwnerThread() const noexcept;

        /**
         * @brief Post a callback to run on next event loop iteration
         * Thread-safe - can be called from background threads
         */
        void post(std::function<void()> callback);

        EventLoopPostHandle getPostHandle() const;

        // Launch background transport work owned by this loop. The worker must
        // not construct Value objects or touch VM state; it communicates back
        // through a captured EventLoopPostHandle.
        bool launchWorker(std::function<void()> worker);

        /**
         * @brief Get a task by ID
         * @param taskId The ID of the task to retrieve
         * @return Shared pointer to the task, or nullptr if not found
         */
        std::shared_ptr<Task> getTask(size_t taskId) const;

        // Visit explicit task state without coupling the event loop to GC.
        // Captures stored inside std::function remain opaque C++ ownership.
        void visitGCRoots(
            const std::function<void(const value::Value&)>& valueVisitor,
            const std::function<void(value::PromiseValue*)>& promiseVisitor) const;

    private:
        void executeTask(std::shared_ptr<Task> task);
        void checkCompletedPromises();
        void moveReadyDelayedTasks();
        std::shared_ptr<Task> selectNextTask();
        void cleanupCompletedTasks();
        void requireOwnerThread(const char* operation) const;
        void reapCompletedWorkers();
        void joinWorkersThrough(uint64_t generation);
        bool hasBackgroundWork() const;
        std::deque<std::function<void()>> takePostedCallbacks();
    };

} // namespace runtime
