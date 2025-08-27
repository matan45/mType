#pragma once
#include <exception>

namespace exception
{
    class ContinueException : public std::exception
    {
    public:
        const char* what() const noexcept override
        {
            return "Continue to next iteration";
        }
    };
}
