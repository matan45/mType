#pragma once
#include <exception>
#include <string>
#include "SourceLocation.hpp"

namespace errors
{
    class ScriptException : public std::exception
    {
    private:
        std::string message;
        SourceLocation location;
        std::string fullMessage;

    public:
        explicit ScriptException(const std::string& msg, const SourceLocation& loc)
            : message(msg), location(loc)
        {
            std::stringstream ss;
            ss << "Error at " << location.toString() << ": " << message;
            fullMessage = ss.str();
        }

        const char* what() const noexcept override
        {
            return fullMessage.c_str();
        }

        const SourceLocation& getLocation() const { return location; }
        const std::string& getMessage() const { return message; }
    };
}
