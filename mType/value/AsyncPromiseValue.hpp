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
        void then(std::function<void(Value)> callback)
        {
            std::lock_guard<std::mutex> lock(callbackMutex);

            if (isFulfilled())
            {
                // Already resolved - execute immediately
                callback(getValue());
            }
            else
            {
                // Store for later execution
                thenCallbacks.push_back(callback);
            }
        }

        /**
         * @brief Register error handler for when promise is rejected
         *
         * If promise is already rejected, callback is executed immediately.
         * Otherwise, callback is stored and executed when reject() is called.
         *
         * @param callback Function to call with error message
         */
        void catch_(std::function<void(std::string)> callback)
        {
            std::lock_guard<std::mutex> lock(callbackMutex);

            if (isRejected())
            {
                // Already rejected - execute immediately
                callback(getError());
            }
            else
            {
                // Store for later execution
                catchCallbacks.push_back(callback);
            }
        }

        /**
         * @brief Register callback that runs regardless of fulfill/reject
         *
         * Useful for cleanup operations that must run in either case.
         *
         * @param callback Function to call when promise settles
         */
        void finally(std::function<void()> callback)
        {
            std::lock_guard<std::mutex> lock(callbackMutex);

            if (isFulfilled() || isRejected())
            {
                // Already settled - execute immediately
                callback();
            }
            else
            {
                // Store for later execution
                finallyCallbacks.push_back(callback);
            }
        }

        /**
         * @brief Resolve promise and notify all waiting tasks
         *
         * Executes all registered .then() callbacks in order.
         * After calling, the promise is in FULFILLED state.
         *
         * @param val The value to resolve with
         * @throws std::runtime_error if promise is already settled
         */
        void resolve(const Value& val)
        {
            {
                std::lock_guard<std::mutex> lock(callbackMutex);

                // Call parent resolve() which validates state
                PromiseValue::resolve(val);

                // Execute all .then() callbacks
                for (auto& callback : thenCallbacks)
                {
                    try
                    {
                        callback(val);
                    }
                    catch (const std::exception&)
                    {

                    }
                }

                // Execute .finally() callbacks
                for (auto& callback : finallyCallbacks)
                {
                    try
                    {
                        callback();
                    }
                    catch (const std::exception&)
                    {
                    }
                }

                // Clear callbacks after execution
                thenCallbacks.clear();
                finallyCallbacks.clear();
            }
        }

        /**
         * @brief Reject promise and notify error handlers
         *
         * Executes all registered .catch() callbacks in order.
         * After calling, the promise is in REJECTED state.
         *
         * @param error Error message describing the rejection
         * @throws std::runtime_error if promise is already settled
         */
        void reject(const std::string& error)
        {
            {
                std::lock_guard<std::mutex> lock(callbackMutex);

                // Call parent reject() which validates state
                PromiseValue::reject(error);

                // Execute all .catch() callbacks
                for (auto& callback : catchCallbacks)
                {
                    try
                    {
                        callback(error);
                    }
                    catch (const std::exception&)
                    {

                    }
                }

                // Execute .finally() callbacks
                for (auto& callback : finallyCallbacks)
                {
                    try
                    {
                        callback();
                    }
                    catch (const std::exception& )
                    {
                    }
                }

                // Clear callbacks after execution
                catchCallbacks.clear();
                finallyCallbacks.clear();
            }
        }

        /**
         * @brief Chain another promise to this one
         *
         * When this promise resolves, the chained promise is resolved with
         * the transformed value. Enables promise chaining pattern.
         *
         * @param transform Function to transform the resolved value
         * @return New promise that will be fulfilled with transformed value
         */
        std::shared_ptr<AsyncPromiseValue> chain(
            std::function<Value(Value)> transform)
        {
            auto chainedPromise = std::make_shared<AsyncPromiseValue>();

            then([chainedPromise, transform](Value result) {
                try
                {
                    Value transformed = transform(result);
                    chainedPromise->resolve(transformed);
                }
                catch (const std::exception& e)
                {
                    chainedPromise->reject(e.what());
                }
            });

            catch_([chainedPromise](std::string error) {
                chainedPromise->reject(error);
            });

            return chainedPromise;
        }

        /**
         * @brief Get number of pending callbacks
         */
        size_t getPendingCallbackCount() const
        {
            std::lock_guard<std::mutex> lock(callbackMutex);
            return thenCallbacks.size() + catchCallbacks.size() + finallyCallbacks.size();
        }
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
