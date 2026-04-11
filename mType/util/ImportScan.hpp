#pragma once
#include <string>

namespace util
{
    /**
     * Scan the first ~50 lines of a source file for `^\s*import\b`
     * and return the 0-based line number where a new import statement
     * should be inserted.
     *
     * Returns the line *after* the last matching import line, or 0 if
     * no imports exist in the scanned window. The 50-line cap keeps
     * the scan cheap for large files where imports always live at the
     * top.
     *
     * MYT-51 — extracted from CodeActionHandler so both the missing-
     * import quick fix and the auto-import completion path produce
     * identical insertion points.
     */
    int findImportInsertLine(const std::string& sourceContent);
}
