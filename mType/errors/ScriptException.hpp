#pragma once
#include <exception>
#include <string>
#include <sstream>
#include <utility>
#include <vector>
#include "SourceLocation.hpp"

namespace errors
{
    class ScriptException : public std::exception
    {
    private:
        std::string message;
        SourceLocation location;
        std::string fullMessage;
        std::vector<std::string> stackTrace_;

        void rebuildFullMessage()
        {
            std::stringstream ss;
            ss << "Error at " << location.toString() << ": " << message;
            fullMessage = ss.str();
        }

    public:
        explicit ScriptException(const std::string& msg, const SourceLocation& loc)
            : message(msg), location(loc)
        {
            rebuildFullMessage();
        }

        const char* what() const noexcept override
        {
            return fullMessage.c_str();
        }

        const SourceLocation& getLocation() const { return location; }
        const std::string& getMessage() const { return message; }

        // MYT-46: post-construction decoration. Only overwrites the location
        // if the current one is the sentinel default — never clobbers a
        // meaningful caller-set location.
        void setLocation(const SourceLocation& loc)
        {
            if (location.getFilename() == "<unknown>"
                && location.getLine() == 1
                && location.getColumn() == 1)
            {
                location = loc;
                rebuildFullMessage();
            }
        }

        void setStackTrace(std::vector<std::string> trace)
        {
            stackTrace_ = std::move(trace);
        }

        const std::vector<std::string>& getStackTrace() const
        {
            return stackTrace_;
        }
    };
}
