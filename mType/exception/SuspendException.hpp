#pragma once
#include <exception>
#include <memory>
#include "../value/ValueType.hpp"
#include "../value/AsyncPromiseValue.hpp"

namespace exception
{
    /**
     * @brief Exception thrown when an async function needs to suspend execution
     *
     * This exception is thrown when await encounters an unfulfilled promise.
     * It unwinds the C++ call stack and allows the EventLoop to register
     * a continuation callback that will resume execution when the promise resolves.
     */
    class SuspendException : public std::exception
    {
    private:
        std::shared_ptr<value::AsyncPromiseValue> promise;
        value::Value resumeValue;
        bool hasResumeValue;

    public:
        explicit SuspendException(std::shared_ptr<value::AsyncPromiseValue> p)
            : promise(p)
            , hasResumeValue(false)
        {
        }

        std::shared_ptr<value::AsyncPromiseValue> getPromise() const
        {
            return promise;
        }

        void setResumeValue(const value::Value& value)
        {
            resumeValue = value;
            hasResumeValue = true;
        }

        value::Value getResumeValue() const
        {
            return resumeValue;
        }

        bool hasValue() const
        {
            return hasResumeValue;
        }

        const char* what() const noexcept override
        {
            return "Task suspended - awaiting promise";
        }
    };
}
