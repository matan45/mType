#pragma once
#include "ScriptException.hpp"

namespace errors
{
    class AmbiguousReferenceException : public ScriptException
    {
    public:
        explicit AmbiguousReferenceException(const std::string& msg,
                                   const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc)
        {
        }
    };
}