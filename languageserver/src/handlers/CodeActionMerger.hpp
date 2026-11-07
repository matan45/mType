#pragma once

#include <vector>

#include "../utils/LSPTypes.hpp"

namespace mtype::lsp {

/**
 * MYT-445 — collapses code actions that would do exactly the same thing.
 *
 * Editors send every diagnostic on the cursor's line, so a type used twice
 * there (`Circle c = new Circle();`) yields two unresolved-type diagnostics
 * and, without this, two identical "Add import 'Circle'" entries. Two
 * actions are duplicates when title, kind and edit all match; actions that
 * differ only in their edit (the same name importable from two files) stay
 * separate. The survivor keeps the first one's position and lists every
 * diagnostic any of the duplicates addressed.
 */
class CodeActionMerger {
public:
    static std::vector<CodeAction> mergeDuplicates(std::vector<CodeAction> actions);
};

} // namespace mtype::lsp
