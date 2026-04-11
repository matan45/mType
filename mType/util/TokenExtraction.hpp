#pragma once
#include <string>
#include <string_view>

namespace util
{
    /**
     * Walk backwards from `column` over identifier characters
     * (`[A-Za-z0-9_]`) and return the prefix of the identifier that
     * ends at `column`.
     *
     * Used by the completion handler to extract the partial token
     * under the cursor so the fuzzy filter can score candidates.
     *
     * Returns empty if:
     *   - `column` is 0, out of range, or the character immediately
     *     before it is not an identifier byte;
     *   - the line is empty.
     *
     * MYT-51 — extracted so the pure logic is testable from the
     * main test runner without spinning up an LSP harness.
     *
     * @param line   Source line (no terminating newline).
     * @param column 0-based column index of the cursor. Points one
     *               past the last identifier byte to include.
     */
    std::string extractIdentifierTokenBefore(std::string_view line, int column);
}
