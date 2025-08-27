#pragma once
#include "ScriptException.hpp"

namespace errors
{
    class UndefinedException : public ScriptException
    {
    public:
        explicit UndefinedException(const std::string& msg,
                                   const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc)
        {
        }
    };
}