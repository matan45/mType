#pragma once
#include "ScriptException.hpp"

namespace errors
{
    class ImportException : public ScriptException
    {
    public:
        explicit ImportException(const std::string& msg,
                                   const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc)
        {
        }
    };
}