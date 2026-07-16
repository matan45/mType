#include "AsyncPromiseValue.hpp"
#include <cstddef>
#include <iostream>
#include <optional>

namespace
{
    template<typename Callback, typename Invoker>
    void invokeCallbacks(
        std::vector<Callback>& callbacks,
        Invoker&& invoke,
        const char* callbackKind,
        std::vector<std::string>& errors)
    {
        for (auto& callback : callbacks)
        {
            try
            {
                invoke(callback);
            }
            catch (const std::exception& exception)
            {
                errors.push_back(
                    "Error in " + std::string(callbackKind) +
                    " callback: " + exception.what());
            }
        }
    }

    void logCallbackErrors(const std::vector<std::string>& errors)
    {
        for (const auto& error : errors)
        {
            std::cerr << "AsyncPromiseValue: " << error << std::endl;
        }
    }
}

namespace value
{
    void AsyncPromiseValue::then(std::function<void(Value)> callback)
    {
        std::optional<Value> settledValue;
        {
            std::lock_guard<std::mutex> lock(callbackMutex);
            const PromiseState currentState = getState();
            if (currentState == PromiseState::FULFILLED)
            {
                settledValue = getValue();
            }
            else if (currentState == PromiseState::PENDING)
            {
                thenCallbacks.push_back(std::move(callback));
                return;
            }
        }

        // Never invoke user code while callbackMutex is held. In particular,
        // the callback may register another callback on this same promise.
        if (settledValue)
        {
            callback(*settledValue);
        }
    }

    void AsyncPromiseValue::catch_(std::function<void(std::string)> callback)
    {
        std::optional<std::string> settledError;
        {
            std::lock_guard<std::mutex> lock(callbackMutex);
            const PromiseState currentState = getState();
            if (currentState == PromiseState::REJECTED)
            {
                settledError = getError();
            }
            else if (currentState == PromiseState::PENDING)
            {
                catchCallbacks.push_back(std::move(callback));
                return;
            }
        }

        if (settledError)
        {
            callback(*settledError);
        }
    }

    void AsyncPromiseValue::finally(std::function<void()> callback)
    {
        bool runNow = false;
        {
            std::lock_guard<std::mutex> lock(callbackMutex);
            runNow = getState() != PromiseState::PENDING;
            if (!runNow)
            {
                finallyCallbacks.push_back(std::move(callback));
                return;
            }
        }

        if (runNow)
        {
            callback();
        }
    }

    void AsyncPromiseValue::resolve(const Value& val)
    {
        std::vector<std::function<void(Value)>> callbacks;
        std::vector<std::function<void()>> finalizers;
        std::vector<std::string> errorsToLog;

        {
            std::lock_guard<std::mutex> lock(callbackMutex);
            PromiseValue::resolve(val);
            callbacks.swap(thenCallbacks);
            finalizers.swap(finallyCallbacks);
            catchCallbacks.clear();
        }

        invokeCallbacks(callbacks, [&val](auto& callback) { callback(val); },
                        ".then()", errorsToLog);
        invokeCallbacks(finalizers, [](auto& callback) { callback(); },
                        ".finally()", errorsToLog);

        if (!errorsToLog.empty())
        {
            std::lock_guard<std::mutex> lock(callbackMutex);
            callbackErrors.insert(
                callbackErrors.end(), errorsToLog.begin(), errorsToLog.end());
        }

        logCallbackErrors(errorsToLog);
    }

    void AsyncPromiseValue::reject(const std::string& error)
    {
        std::vector<std::function<void(std::string)>> callbacks;
        std::vector<std::function<void()>> finalizers;
        std::vector<std::string> errorsToLog;

        {
            std::lock_guard<std::mutex> lock(callbackMutex);
            PromiseValue::reject(error);
            callbacks.swap(catchCallbacks);
            finalizers.swap(finallyCallbacks);
            thenCallbacks.clear();
        }

        invokeCallbacks(callbacks, [&error](auto& callback) { callback(error); },
                        ".catch()", errorsToLog);
        invokeCallbacks(finalizers, [](auto& callback) { callback(); },
                        ".finally()", errorsToLog);

        if (!errorsToLog.empty())
        {
            std::lock_guard<std::mutex> lock(callbackMutex);
            callbackErrors.insert(
                callbackErrors.end(), errorsToLog.begin(), errorsToLog.end());
        }

        logCallbackErrors(errorsToLog);
    }

    void AsyncPromiseValue::rejectWithException(const Value& exceptionVal, const std::string& typeName, const std::string& error)
    {
        std::vector<std::function<void(std::string)>> callbacks;
        std::vector<std::function<void()>> finalizers;
        std::vector<std::string> errorsToLog;

        {
            std::lock_guard<std::mutex> lock(callbackMutex);
            PromiseValue::rejectWithException(exceptionVal, typeName, error);
            callbacks.swap(catchCallbacks);
            finalizers.swap(finallyCallbacks);
            thenCallbacks.clear();
        }

        invokeCallbacks(callbacks, [&error](auto& callback) { callback(error); },
                        ".catch()", errorsToLog);
        invokeCallbacks(finalizers, [](auto& callback) { callback(); },
                        ".finally()", errorsToLog);

        if (!errorsToLog.empty())
        {
            std::lock_guard<std::mutex> lock(callbackMutex);
            callbackErrors.insert(
                callbackErrors.end(), errorsToLog.begin(), errorsToLog.end());
        }

        logCallbackErrors(errorsToLog);
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
