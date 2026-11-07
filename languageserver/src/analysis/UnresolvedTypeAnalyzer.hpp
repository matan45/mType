#pragma once

#include <vector>

#include "../../../mType/diagnostics/Diagnostic.hpp"

namespace mtype::lsp {

struct Document;

/**
 * MYT-440 — unresolved-type diagnostics while typing.
 *
 * The AST decides *which* names are unresolved: a name used in a checked
 * type position (TypeReferenceCollector) that is not known
 * (KnownTypeNames) and is not also declared as a value. The token stream
 * decides *where*: every identifier token with that name gets an
 * MT-E1003 diagnostic covering exactly the token, except a member name
 * (after `.`, `?.` or `::`), an annotation (after `@`), and anything inside
 * an import statement. Tokens are matched by text, so a range can only
 * land on the name itself.
 *
 * Each diagnostic carries `exceptionType: "ClassNotFoundException"` and
 * `missingName` in its LSP data, which is what the MYT-47 "Add import"
 * quick fix keys on (MYT-445).
 *
 * The caller must only run this on a document whose current content parsed
 * and whose imports all resolved (ImportSummary::complete); otherwise a
 * type could come from something the LS could not read.
 */
class UnresolvedTypeAnalyzer {
public:
    // Never throws; returns nothing if the analysis can't run.
    static std::vector<diagnostics::Diagnostic> analyze(const Document& doc);
};

} // namespace mtype::lsp
