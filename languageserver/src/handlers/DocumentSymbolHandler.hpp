#pragma once

#include <string>
#include <vector>

#include "../DocumentManager.hpp"
#include "../utils/LSPTypes.hpp"

namespace mtype::lsp {

// MYT-296 — Document symbols for the editor outline / breadcrumb bar.
//
// Walks the parsed AST for a single document and produces a
// hierarchical DocumentSymbol[]. Top-level surfaces are classes,
// interfaces, and functions; class members (constructors, fields,
// methods) are nested as children of their owning class; interface
// method signatures are nested under their interface.
//
// Top-level variables are intentionally NOT surfaced in v1: the parser
// emits `AssignmentNode` for both `Int a = 5;` declarations and `a = 5;`
// reassignments with no `isDeclaration` flag, and surfacing them
// reliably needs a dedicated VariableDeclarationNode. The existing
// WorkspaceSymbolIndex and SymbolRegistrationVisitor both skip
// variables for the same reason.
//
// Robustness: the handler keeps every AST access behind a try/catch and
// degrades to an empty vector on parse failure or unexpected state.
// DocumentManager preserves the previously valid AST on a fresh parse
// failure, so the outline naturally freezes on the last good state
// while the user is mid-edit.
class DocumentSymbolHandler {
public:
    explicit DocumentSymbolHandler(DocumentManager* docMgr);

    std::vector<DocumentSymbol> handleDocumentSymbol(const std::string& uri);

private:
    DocumentManager* documentManager_;
};

} // namespace mtype::lsp
