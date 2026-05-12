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
// Empty query returns an empty list — the underlying `findByPrefix`
// short-circuits on an empty prefix to avoid streaming the entire
// workspace symbol pool. This matches what VS Code's Ctrl+T expects:
// nothing is shown until the user types at least one character, the
// same threshold completion already uses.
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
