#pragma once
#include "ScriptException.hpp"

namespace errors
{
    class EnvironmentException : public ScriptException
    {
    public:
        explicit EnvironmentException(const std::string& msg,
                                   const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc)
        {
        }
    };
}