#include "WorkspaceSymbolHandler.hpp"

#include <chrono>
#include <utility>

namespace mtype::lsp {

namespace {

constexpr std::size_t kMaxResults = 256;
constexpr auto kReadyWait = std::chrono::milliseconds(50);

SymbolKind toLspKind(analysis::WorkspaceSymbolKind kind) {
    switch (kind) {
        case analysis::WorkspaceSymbolKind::Class:     return SymbolKind::Class;
        case analysis::WorkspaceSymbolKind::Interface: return SymbolKind::Interface;
        case analysis::WorkspaceSymbolKind::Function:  return SymbolKind::Function;
        case analysis::WorkspaceSymbolKind::Unknown:
        default:
            return SymbolKind::Variable;
    }
}

} // namespace

WorkspaceSymbolHandler::WorkspaceSymbolHandler(
    std::shared_ptr<analysis::WorkspaceSymbolIndex> index)
    : workspaceIndex_(std::move(index)) {}

std::vector<SymbolInformation> WorkspaceSymbolHandler::handleWorkspaceSymbol(
    const std::string& query) {
    std::vector<SymbolInformation> out;
    if (!workspaceIndex_) {
        return out;
    }

    // Short-block early requests against the initial workspace scan,
    // then query whatever the index has — partial results are better
    // than blocking the editor.
    workspaceIndex_->waitForReady(kReadyWait);

    auto matches = workspaceIndex_->findByPrefix(query, kMaxResults);
    out.reserve(matches.size());

    for (const auto& sym : matches) {
        if (sym.kind == analysis::WorkspaceSymbolKind::Unknown) {
            continue;
        }

        // The index stores a single (line, column) anchor; expand it
        // into a degenerate range covering just the symbol name so
        // clients have something to highlight when they jump to it.
        const int endChar = sym.column + static_cast<int>(sym.name.size());
        Position start{sym.line, sym.column};
        Position end{sym.line, endChar};

        SymbolInformation info;
        info.name = sym.name;
        info.kind = toLspKind(sym.kind);
        info.location.uri = sym.fileUri;
        info.location.range = Range{start, end};
        out.push_back(std::move(info));
    }

    return out;
}

} // namespace mtype::lsp
