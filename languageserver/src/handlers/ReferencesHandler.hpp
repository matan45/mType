#pragma once

#include <string>
#include <vector>
#include "../utils/LSPTypes.hpp"
#include "../DocumentManager.hpp"
#include "../analysis/WorkspaceSymbolIndex.hpp"

namespace mtype::lsp {

// Find-references handler for textDocument/references.
//
// Methods and free/static functions are matched against the parsed AST
// so that a search for `Foo.bar` only returns `Foo.bar` (and overrides),
// not every `bar` token in the file. Classes and bare variables still
// use a textual whole-word scan; classes need it to match type
// annotations / extends / implements, and variable scope inference is
// out of scope for this handler (rename owns that).
//
// Scope: every document currently open in DocumentManager. Files that
// the editor hasn't opened are not scanned — adding a parse-on-demand
// path is follow-up work; the same constraint applies to the call
// hierarchy handler.
class ReferencesHandler {
public:
    ReferencesHandler(DocumentManager* docMgr,
                      std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex = nullptr);

    std::vector<Location> handleReferences(
        const std::string& uri,
        const Position& position,
        bool includeDeclaration
    );

private:
    DocumentManager* documentManager_;
    std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex_;
};

} // namespace mtype::lsp
