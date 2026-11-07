#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../analysis/WorkspaceSymbolIndex.hpp"
#include "../utils/LSPTypes.hpp"

namespace mtype::lsp {

// MYT-297 — Workspace-wide symbol search (workspace/symbol).
//
// Thin adapter over the existing WorkspaceSymbolIndex (MYT-47): the
// index already maintains the name → file/location map that powers the
// missing-import quick fix and the auto-import completion branch, and
// the same prefix matcher (case-insensitive, alphabetic-then-uri sort)
// is reused here so workspace symbol search ranks results consistently
// with completion.
//
// Contract (MYT-447): an empty query returns every top-level symbol,
// sorted alphabetically (case-insensitive) and capped at 256, via
// `WorkspaceSymbolIndex::listAll`. A non-empty query is a
// case-insensitive prefix match on the name, ranked exact-match first,
// with the same cap. The mType editor's Ctrl+T picker shows the
// empty-query list when it opens and re-queries as the user types;
// VS Code re-queries on every keystroke.
//
// Early requests, before the initial workspace scan has finished, fall
// through to a 50 ms short-block via `waitForReady` (same ceiling used
// by the code-action and completion handlers). On timeout the index
// returns whatever it has so far rather than blocking the editor.
class WorkspaceSymbolHandler {
public:
    explicit WorkspaceSymbolHandler(
        std::shared_ptr<analysis::WorkspaceSymbolIndex> index);

    std::vector<SymbolInformation> handleWorkspaceSymbol(
        const std::string& query);

private:
    std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex_;
};

} // namespace mtype::lsp
