#pragma once
#include "ScriptException.hpp"

namespace errors
{
    class ArgumentException : public ScriptException
    {
    public:
        explicit ArgumentException(const std::string& msg,
                                   const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc)
        {
        }
    };
}
