#pragma once
#include <exception>

namespace errors
{
    class BreakException : public std::exception
    {
    public:
        const char* what() const noexcept override
        {
            return "Break from loop";
        }
    };
}
