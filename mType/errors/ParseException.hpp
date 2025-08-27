#pragma once
#include "ScriptException.hpp"

namespace errors
{
    class ParseException : public ScriptException
    {
    public:
        explicit ParseException(const std::string& msg,
                                   const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc)
        {
        }
    };
}