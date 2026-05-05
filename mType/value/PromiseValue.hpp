#pragma once
#include "ValueType.hpp"
#include <chrono>
#include <condition_variable>

namespace value
{
    /**
     * @brief State of a Promise
     */
    enum class PromiseState
    {
        PENDING,   // Promise is not yet resolved
        FULFILLED, // Promise resolved successfully with a value
        REJECTED   // Promise rejected with an error (future use)
    };

    /**
     * @brief Runtime representation of a Promise<T>
     *
     * For Phase 2 (Evaluator), we use a simple synchronous model:
     * - await immediately resolves the promise and returns its value
     * - Promises are always in FULFILLED state after async function execution
     *
     * This is a simplified model suitable for single-threaded cooperative execution.
     * True asynchronous behavior would require event loops and scheduling.
     */
    class PromiseValue
    {
    private:
        mutable std::mutex promiseMutex;  // Protects state and value for thread safety
        mutable std::condition_variable cv;  // For efficient blocking on promise resolution
        PromiseState state;
        Value value;
        std::string errorMessage; // For future error handling
        Value exceptionValue; // Store the actual exception object when rejected
        std::string exceptionTypeName; // Type name of the exception

    public:
        /**
         * @brief Create a pending promise
         */
        PromiseValue()
            : state(PromiseState::PENDING)
            , value(std::monostate{})
            , exceptionValue(std::monostate{})
        {
        }

        /**
         * @brief Create a fulfilled promise with a value
         */
        explicit PromiseValue(const Value& val)
            : state(PromiseState::FULFILLED)
            , value(val)
            , exceptionValue(std::monostate{})
        {
        }

        /**
         * @brief Virtual destructor for polymorphism
         */
        virtual ~PromiseValue() = default;

        /**
         * @brief Get the current state of the promise
         */
        PromiseState getState() const {
            std::lock_guard<std::mutex> lock(promiseMutex);
            return state;
        }

        /**
         * @brief Check if promise is fulfilled
         */
        bool isFulfilled() const {
            std::lock_guard<std::mutex> lock(promiseMutex);
            return state == PromiseState::FULFILLED;
        }

        /**
         * @brief Check if promise is pending
         */
        bool isPending() const {
            std::lock_guard<std::mutex> lock(promiseMutex);
            return state == PromiseState::PENDING;
        }

        /**
         * @brief Check if promise is rejected
         */
        bool isRejected() const {
            std::lock_guard<std::mutex> lock(promiseMutex);
            return state == PromiseState::REJECTED;
        }

        /**
         * @brief Resolve the promise with a value
         * @throws std::runtime_error if promise is already settled (fulfilled or rejected)
         */
        virtual void resolve(const Value& val)
        {
            {
                std::lock_guard<std::mutex> lock(promiseMutex);
                if (state != PromiseState::PENDING)
                {
                    throw std::runtime_error(
                        "Cannot resolve promise: Promise is already " +
                        std::string(state == PromiseState::FULFILLED ? "fulfilled" : "rejected")
                    );
                }
                state = PromiseState::FULFILLED;
                value = val;
            }
            // Notify all threads waiting on this promise (outside lock to avoid deadlock)
            cv.notify_all();
        }

        /**
         * @brief Reject the promise with an error
         * @throws std::runtime_error if promise is already settled (fulfilled or rejected)
         */
        virtual void reject(const std::string& error)
        {
            {
                std::lock_guard<std::mutex> lock(promiseMutex);
                if (state != PromiseState::PENDING)
                {
                    throw std::runtime_error(
                        "Cannot reject promise: Promise is already " +
                        std::string(state == PromiseState::FULFILLED ? "fulfilled" : "rejected")
                    );
                }
                state = PromiseState::REJECTED;
                errorMessage = error;
            }
            // Notify all threads waiting on this promise (outside lock to avoid deadlock)
            cv.notify_all();
        }

        /**
         * @brief Reject the promise with an exception object
         * @param exceptionVal The exception value object
         * @param typeName The type name of the exception
         * @param error The error message
         * @throws std::runtime_error if promise is already settled (fulfilled or rejected)
         */
        virtual void rejectWithException(const Value& exceptionVal, const std::string& typeName, const std::string& error)
        {
            {
                std::lock_guard<std::mutex> lock(promiseMutex);
                if (state != PromiseState::PENDING)
                {
                    throw std::runtime_error(
                        "Cannot reject promise: Promise is already " +
                        std::string(state == PromiseState::FULFILLED ? "fulfilled" : "rejected")
                    );
                }
                state = PromiseState::REJECTED;
                exceptionValue = exceptionVal;
                exceptionTypeName = typeName;
                errorMessage = error;
            }
            // Notify all threads waiting on this promise (outside lock to avoid deadlock)
            cv.notify_all();
        }

        /**
         * @brief Get the resolved value
         * @throws std::runtime_error if promise is not fulfilled
         */
        Value getValue() const
        {
            std::lock_guard<std::mutex> lock(promiseMutex);
            if (state != PromiseState::FULFILLED)
            {
                throw std::runtime_error("Cannot get value from non-fulfilled promise");
            }
            return value;
        }

        /**
         * @brief Get error message if rejected
         */
        std::string getError() const
        {
            std::lock_guard<std::mutex> lock(promiseMutex);
            return errorMessage;
        }

        /**
         * @brief Get the exception value if rejected with an exception
         */
        Value getExceptionValue() const
        {
            std::lock_guard<std::mutex> lock(promiseMutex);
            return exceptionValue;
        }

        /**
         * @brief Get the exception type name if rejected with an exception
         */
        std::string getExceptionTypeName() const
        {
            std::lock_guard<std::mutex> lock(promiseMutex);
            return exceptionTypeName;
        }

        /**
         * @brief Clear held values for GC cycle breaking
         * Resets value and exceptionValue to monostate to release object references.
         */
        void clearForGC()
        {
            std::lock_guard<std::mutex> lock(promiseMutex);
            value = std::monostate{};
            exceptionValue = std::monostate{};
        }

        /**
         * @brief Efficiently wait for promise to be fulfilled or rejected
         *
         * Blocks the calling thread until the promise is settled (fulfilled or rejected).
         * Uses condition variable for efficient blocking with zero CPU usage.
         * This replaces the inefficient busy-wait polling pattern.
         *
         * @return The resolved value if fulfilled
         * @throws std::runtime_error if promise is rejected or null
         */
        Value waitForValue() const
        {
            std::unique_lock<std::mutex> lock(promiseMutex);

            // Wait until promise is no longer pending
            cv.wait(lock, [this] { return state != PromiseState::PENDING; });

            // Check if promise was rejected
            if (state == PromiseState::REJECTED)
            {
                throw std::runtime_error("Promise was rejected: " + errorMessage);
            }

            // Promise is fulfilled, return the value
            return value;
        }

        /**
         * @brief Wait for promise with timeout
         *
         * @param timeoutMs Maximum time to wait in milliseconds
         * @return The resolved value if fulfilled within timeout
         * @throws std::runtime_error if timeout, rejected, or null
         */
        Value waitForValue(int timeoutMs) const
        {
            std::unique_lock<std::mutex> lock(promiseMutex);

            // Wait with timeout
            bool completed = cv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                [this] { return state != PromiseState::PENDING; });

            if (!completed)
            {
                throw std::runtime_error("Timeout waiting for promise to be fulfilled");
            }

            // Check if promise was rejected
            if (state == PromiseState::REJECTED)
            {
                throw std::runtime_error("Promise was rejected: " + errorMessage);
            }

            // Promise is fulfilled, return the value
            return value;
        }
    };
}
