#pragma once
#include "ParseException.hpp"

namespace errors
{
    /**
     * @brief Thrown when the parser expected `;` and found a different token.
     *
     * Carried as a typed subclass of `ParseException` so the diagnostic
     * converter can route to `MT-E0002 ParseMissingSemicolon` and the LSP
     * code action handler can offer "Insert ';'" as a quick fix.
     *
     * The location is the start of the token that appeared in place of
     * the semicolon — i.e., the byte right after where `;` should have
     * been written. The quick fix inserts `;` at exactly this column.
     */
    class MissingSemicolonException : public ParseException
    {
    public:
        explicit MissingSemicolonException(const SourceLocation& loc)
            : ParseException("expected ';' at end of statement", loc)
        {
        }
    };
}
