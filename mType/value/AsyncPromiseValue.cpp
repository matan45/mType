#include "AsyncPromiseValue.hpp"
#include <cstddef>
#include <iostream>
namespace value
{
    void AsyncPromiseValue::then(std::function<void(Value)> callback)
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

    void AsyncPromiseValue::catch_(std::function<void(std::string)> callback)
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

    void AsyncPromiseValue::finally(std::function<void()> callback)
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

    void AsyncPromiseValue::resolve(const Value& val)
    {
        // Local storage for errors to log outside the lock
        std::vector<std::string> errorsToLog;

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
                catch (const std::exception& e)
                {
                    std::string errorMsg = "Error in .then() callback: " + std::string(e.what());
                    callbackErrors.push_back(errorMsg);
                    errorsToLog.push_back(errorMsg);
                }
            }

            // Execute .finally() callbacks
            for (auto& callback : finallyCallbacks)
            {
                try
                {
                    callback();
                }
                catch (const std::exception& e)
                {
                    std::string errorMsg = "Error in .finally() callback: " + std::string(e.what());
                    callbackErrors.push_back(errorMsg);
                    errorsToLog.push_back(errorMsg);
                }
            }

            // Clear callbacks after execution
            thenCallbacks.clear();
            finallyCallbacks.clear();
        }

        // Log errors outside the lock to avoid I/O under lock
        for (const auto& error : errorsToLog)
        {
            std::cerr << "AsyncPromiseValue: " << error << std::endl;
        }
    }

    void AsyncPromiseValue::reject(const std::string& error)
    {
        // Local storage for errors to log outside the lock
        std::vector<std::string> errorsToLog;

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
                catch (const std::exception& e)
                {
                    std::string errorMsg = "Error in .catch() callback: " + std::string(e.what());
                    callbackErrors.push_back(errorMsg);
                    errorsToLog.push_back(errorMsg);
                }
            }

            // Execute .finally() callbacks
            for (auto& callback : finallyCallbacks)
            {
                try
                {
                    callback();
                }
                catch (const std::exception& e)
                {
                    std::string errorMsg = "Error in .finally() callback: " + std::string(e.what());
                    callbackErrors.push_back(errorMsg);
                    errorsToLog.push_back(errorMsg);
                }
            }

            // Clear callbacks after execution
            catchCallbacks.clear();
            finallyCallbacks.clear();
        }

        // Log errors outside the lock to avoid I/O under lock
        for (const auto& error : errorsToLog)
        {
            std::cerr << "AsyncPromiseValue: " << error << std::endl;
        }
    }

    void AsyncPromiseValue::rejectWithException(const Value& exceptionVal, const std::string& typeName, const std::string& error)
    {
        // Local storage for errors to log outside the lock
        std::vector<std::string> errorsToLog;

        {
            std::lock_guard<std::mutex> lock(callbackMutex);

            // Call parent rejectWithException() which validates state and stores exception
            PromiseValue::rejectWithException(exceptionVal, typeName, error);

            // Execute all .catch() callbacks
            for (auto& callback : catchCallbacks)
            {
                try
                {
                    callback(error);
                }
                catch (const std::exception& e)
                {
                    std::string errorMsg = "Error in .catch() callback: " + std::string(e.what());
                    callbackErrors.push_back(errorMsg);
                    errorsToLog.push_back(errorMsg);
                }
            }

            // Execute .finally() callbacks
            for (auto& callback : finallyCallbacks)
            {
                try
                {
                    callback();
                }
                catch (const std::exception& e)
                {
                    std::string errorMsg = "Error in .finally() callback: " + std::string(e.what());
                    callbackErrors.push_back(errorMsg);
                    errorsToLog.push_back(errorMsg);
                }
            }

            // Clear callbacks after execution
            catchCallbacks.clear();
            finallyCallbacks.clear();
        }

        // Log errors outside the lock to avoid I/O under lock
        for (const auto& error : errorsToLog)
        {
            std::cerr << "AsyncPromiseValue: " << error << std::endl;
        }
    }

    std::shared_ptr<AsyncPromiseValue> AsyncPromiseValue::chain(
        std::function<Value(Value)> transform)
    {
        auto chainedPromise = std::make_shared<AsyncPromiseValue>();

        then([chainedPromise, transform](Value result)
        {
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

        catch_([chainedPromise](std::string error)
        {
            chainedPromise->reject(error);
        });

        return chainedPromise;
    }

    size_t AsyncPromiseValue::getPendingCallbackCount() const
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        return thenCallbacks.size() + catchCallbacks.size() + finallyCallbacks.size();
    }

    std::vector<std::string> AsyncPromiseValue::getCallbackErrors() const
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        return callbackErrors;
    }

    bool AsyncPromiseValue::hasCallbackErrors() const
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        return !callbackErrors.empty();
    }

    void AsyncPromiseValue::clearCallbackErrors()
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        callbackErrors.clear();
    }
}
