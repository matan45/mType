#pragma once
#include "PromiseValue.hpp"
#include <vector>
#include <functional>
#include <mutex>

namespace value
{
    /**
     * @brief Async Promise with callback support for event loop integration
     *
     * Extends PromiseValue to support registering callbacks that are invoked
     * when the promise is fulfilled or rejected. This enables cooperative
     * multitasking where suspended tasks can be resumed when promises resolve.
     *
     * Design:
     * - Callbacks are executed immediately if promise is already settled
     * - Multiple callbacks can be registered (chaining)
     * - Thread-safe callback registration and execution
     * - Integrates with EventLoop for task resumption
     *
     * Usage:
     * @code
     * auto promise = std::make_shared<AsyncPromiseValue>();
     *
     * promise->then([](Value result) {
     *     print("Promise resolved with: " + toString(result));
     * });
     *
     * promise->catch_([](std::string error) {
     *     print("Promise rejected: " + error);
     * });
     *
     * // Later, from async operation:
     * promise->resolve(42);  // Triggers all .then() callbacks
     * @endcode
     */
    class AsyncPromiseValue : public PromiseValue
    {
    private:
        std::vector<std::function<void(Value)>> thenCallbacks;
        std::vector<std::function<void(std::string)>> catchCallbacks;
        std::vector<std::function<void()>> finallyCallbacks;

        // Store errors that occur during callback execution
        std::vector<std::string> callbackErrors;

        // Mutex for thread-safe callback management
        mutable std::mutex callbackMutex;

    public:
        /**
         * @brief Create a pending async promise
         */
        AsyncPromiseValue()
            : PromiseValue()
        {
        }

        /**
         * @brief Create a fulfilled async promise
         */
        explicit AsyncPromiseValue(const Value& val)
            : PromiseValue(val)
        {
        }

        /**
         * @brief Register callback for when promise is fulfilled
         *
         * If promise is already fulfilled, callback is executed immediately.
         * Otherwise, callback is stored and executed when resolve() is called.
         *
         * @param callback Function to call with resolved value
         */
        void then(std::function<void(Value)> callback);

        /**
         * @brief Register error handler for when promise is rejected
         *
         * If promise is already rejected, callback is executed immediately.
         * Otherwise, callback is stored and executed when reject() is called.
         *
         * @param callback Function to call with error message
         */
        void catch_(std::function<void(std::string)> callback);

        /**
         * @brief Register callback that runs regardless of fulfill/reject
         *
         * Useful for cleanup operations that must run in either case.
         *
         * @param callback Function to call when promise settles
         */
        void finally(std::function<void()> callback);

        /**
         * @brief Resolve promise and notify all waiting tasks
         *
         * Executes all registered .then() callbacks in order.
         * After calling, the promise is in FULFILLED state.
         *
         * @param val The value to resolve with
         * @throws std::runtime_error if promise is already settled
         */
        void resolve(const Value& val);

        /**
         * @brief Reject promise and notify error handlers
         *
         * Executes all registered .catch() callbacks in order.
         * After calling, the promise is in REJECTED state.
         *
         * @param error Error message describing the rejection
         * @throws std::runtime_error if promise is already settled
         */
        void reject(const std::string& error);

        /**
         * @brief Chain another promise to this one
         *
         * When this promise resolves, the chained promise is resolved with
         * the transformed value. Enables promise chaining pattern.
         *
         * @param transform Function to transform the resolved value
         * @return New promise that will be fulfilled with transformed value
         */
        std::shared_ptr<AsyncPromiseValue> chain(std::function<Value(Value)> transform);

        /**
         * @brief Get number of pending callbacks
         */
        size_t getPendingCallbackCount() const;

        /**
         * @brief Get all errors that occurred during callback execution
         *
         * Returns a list of error messages from callbacks that threw exceptions.
         * Useful for debugging and error reporting in async code.
         *
         * @return Vector of error messages
         */
        std::vector<std::string> getCallbackErrors() const;

        /**
         * @brief Check if any callbacks threw exceptions
         *
         * @return true if there were callback errors, false otherwise
         */
        bool hasCallbackErrors() const;

        /**
         * @brief Clear all stored callback errors
         *
         * Useful for resetting error state between operations.
         */
        void clearCallbackErrors();
    };

    /**
     * @brief Create a pre-resolved promise
     */
    inline std::shared_ptr<AsyncPromiseValue> makeResolvedPromise(const Value& value)
    {
        return std::make_shared<AsyncPromiseValue>(value);
    }

    /**
     * @brief Create a pre-rejected promise
     */
    inline std::shared_ptr<AsyncPromiseValue> makeRejectedPromise(const std::string& error)
    {
        auto promise = std::make_shared<AsyncPromiseValue>();
        promise->reject(error);
        return promise;
    }
} // namespace value
