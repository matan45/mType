#pragma once
#include <string>
#include <vector>
#include "ScriptException.hpp"

namespace errors
{
    class UndefinedException : public ScriptException
    {
    private:
        // MYT-35 Phase 4 — optional snapshot of identifiers visible at the
        // failure site. Populated by throwers that have scope context, so
        // the diagnostic converter can run a Levenshtein "did you mean"
        // search without having to plumb the environment back into the
        // catch site. Empty when no snapshot is available — the converter
        // simply skips the suggestion in that case.
        std::vector<std::string> identifierPool;

    public:
        explicit UndefinedException(const std::string& msg,
                                   const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc)
        {
        }

        UndefinedException(const std::string& msg,
                           const SourceLocation& loc,
                           std::vector<std::string> pool)
            : ScriptException(msg, loc),
              identifierPool(std::move(pool))
        {
        }

        const std::vector<std::string>& getIdentifierPool() const { return identifierPool; }
    };
}