#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../DocumentManager.hpp"
#include "../utils/LSPTypes.hpp"

namespace mtype::lsp::analysis
{
    class WorkspaceSymbolIndex;
}

namespace mtype::lsp
{
    // Workspace-driven auto-import completion items. Queries the workspace
    // symbol index for names matching a typed prefix that are defined in
    // other files, attaching an additionalTextEdits import line. Also
    // exposes a wrapper-import enrichment used by the primitive-wrapper
    // completion items (Int / Float / Bool / String / Promise) that live
    // in lib/primitives/*.mt.
    class AutoImportCompletionProvider
    {
    public:
        AutoImportCompletionProvider(
            DocumentManager* docMgr,
            std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex);

        // No-op when `workspaceIndex_` is null or `typedPrefix` is empty.
        std::vector<CompletionItem> getAutoImportCompletions(
            const std::string& uri,
            const std::string& typedPrefix,
            const std::vector<CompletionItem>& existingItems) const;

        // Attach an Auto-import detail + import-line edit to `item` for a
        // primitive wrapper class living in another file. No-op when the
        // wrapper is already in scope, the workspace index isn't ready, or
        // the wrapper resolves to the same file as the cursor's document.
        void enrichWithWrapperImport(
            CompletionItem& item,
            const std::string& wrapper,
            const std::string& uri) const;

    private:
        DocumentManager* documentManager_;
        std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex_;
    };
}
