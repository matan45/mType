#pragma once
#include "ScriptException.hpp"

namespace errors
{
    class FileException : public ScriptException
    {
    public:
        explicit FileException(const std::string& msg,
                                   const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc)
        {
        }
    };
}