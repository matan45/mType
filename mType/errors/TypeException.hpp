#pragma once
#include "ScriptException.hpp"

namespace errors
{
    class TypeException : public ScriptException
    {
    public:
        explicit TypeException(const std::string& msg,
                               const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc)
        {
        }
    };
}
