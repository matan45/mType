#pragma once
#include <exception>
#include "../value/ValueType.hpp"

namespace errors
{
    using namespace value;
    // Return exception for early function returns
    class ReturnException : public std::exception
    {
    public:
        Value returnValue;

        explicit ReturnException(const Value& val) : returnValue(val)
        {
        }

        const char* what() const noexcept override
        {
            return "Return from function";
        }
    };
}
