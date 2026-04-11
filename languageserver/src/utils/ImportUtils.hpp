#pragma once

#include <string>

#include "LSPTypes.hpp"

namespace mtype::lsp::utils
{
    /**
     * Build a TextEdit that inserts `import <identifier> from "<spelling>";`
     * at `insertLine` (0-based), as a zero-width edit at column 0.
     *
     * MYT-51 — shared between CodeActionHandler's missing-import quick
     * fix and CompletionHandler's auto-import completion branch so both
     * sites produce byte-identical edits.
     */
    TextEdit buildImportInsertEdit(
        int insertLine,
        const std::string& identifier,
        const std::string& spelling);
}
