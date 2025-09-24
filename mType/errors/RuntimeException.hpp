#pragma once
#include "ScriptException.hpp"

namespace errors
{
    class RuntimeException : public ScriptException
    {
    public:
        explicit RuntimeException(const std::string& msg,
                                const SourceLocation& loc = SourceLocation())
            : ScriptException("Runtime error: " + msg, loc)
        {
        }
    };
}