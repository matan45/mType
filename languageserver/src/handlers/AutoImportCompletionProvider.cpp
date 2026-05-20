#include "AutoImportCompletionProvider.hpp"

#include "internal/CompletionHelpers.hpp"

#include "../analysis/WorkspaceSymbolIndex.hpp"
#include "../utils/ImportUtils.hpp"
#include "../utils/StringUtils.hpp"
#include "../utils/UriUtils.hpp"
#include "../../../mType/environment/registry/ClassRegistry.hpp"
#include "../../../mType/util/ImportScan.hpp"

#include <chrono>
#include <optional>
#include <unordered_set>
#include <utility>

namespace mtype::lsp
{
    namespace
    {
        std::optional<CompletionItem> buildAutoImportItem(
            const analysis::WorkspaceSymbol& match,
            const std::string& referencingPath,
            int insertLine,
            const std::unordered_set<std::string>& existingLabels,
            std::unordered_set<std::string>& emittedAutoImports)
        {
            const std::string loweredName = utils::toLowerAscii(match.name);
            if (existingLabels.count(loweredName) != 0) return std::nullopt;

            const std::string symbolPath = UriUtils::uriToFilePath(match.fileUri);
            if (symbolPath == referencingPath) return std::nullopt;

            const std::string spelling =
                analysis::WorkspaceSymbolIndex::computeImportSpelling(
                    symbolPath, referencingPath);

            const std::string duplicateKey = loweredName + "\n" + spelling;
            if (!emittedAutoImports.insert(duplicateKey).second) return std::nullopt;

            CompletionItem item;
            item.label = match.name;
            item.kind = static_cast<int>(internal::workspaceKindToCompletionKind(match.kind));
            item.detail = "Auto-import from \"" + spelling + "\"";
            item.insertText = match.name;
            item.filterText = match.name;
            item.additionalTextEdits.push_back(
                utils::buildImportInsertEdit(insertLine, match.name, spelling));
            return item;
        }
    }

    AutoImportCompletionProvider::AutoImportCompletionProvider(
        DocumentManager* docMgr,
        std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex)
        : documentManager_(docMgr),
          workspaceIndex_(std::move(workspaceIndex)) {}

    std::vector<CompletionItem> AutoImportCompletionProvider::getAutoImportCompletions(
        const std::string& uri,
        const std::string& typedPrefix,
        const std::vector<CompletionItem>& existingItems) const
    {
        std::vector<CompletionItem> items;

        if (!workspaceIndex_ || typedPrefix.empty()) return items;

        auto doc = documentManager_->getDocument(uri);
        if (!doc) return items;

        // Non-blocking readiness check. A previous `waitForReady(50ms)`
        // synchronously pinned the LSP dispatch thread on every keystroke
        // during the initial workspace scan, which the user felt as a
        // sluggish dropdown. Now we just skip auto-import this round if
        // the index isn't ready — once the scan finishes (next keystroke
        // or cursor move) auto-imports start surfacing.
        if (!workspaceIndex_->waitForReady(std::chrono::milliseconds(0))) return items;

        auto matches = workspaceIndex_->findByPrefix(typedPrefix, /*maxResults=*/20);
        if (matches.empty()) return items;

        // Names already surfaced by the in-scope enumeration — skip them
        // so the dropdown doesn't double-list the same symbol.
        std::unordered_set<std::string> existingLabels;
        existingLabels.reserve(existingItems.size());
        for (const auto& item : existingItems) {
            existingLabels.insert(utils::toLowerAscii(item.label));
        }

        const std::string referencingPath = UriUtils::uriToFilePath(uri);
        const int insertLine = util::findImportInsertLine(doc->content);
        std::unordered_set<std::string> emittedAutoImports;
        for (const auto& match : matches) {
            if (auto item = buildAutoImportItem(match, referencingPath, insertLine,
                                                existingLabels, emittedAutoImports)) {
                items.push_back(std::move(*item));
            }
        }
        return items;
    }

    void AutoImportCompletionProvider::enrichWithWrapperImport(
        CompletionItem& item,
        const std::string& wrapper,
        const std::string& uri) const
    {
        if (!workspaceIndex_) return;

        auto doc = documentManager_ ? documentManager_->getDocument(uri) : nullptr;
        auto classRegistry = (doc && doc->environment)
            ? doc->environment->getClassRegistry() : nullptr;

        // If the wrapper is already imported (visible via the class
        // registry), no auto-import edit needed.
        if (classRegistry && classRegistry->hasClass(wrapper)) return;

        // Non-blocking — same rationale as getAutoImportCompletions above.
        if (!workspaceIndex_->waitForReady(std::chrono::milliseconds(0))) return;

        auto matches = workspaceIndex_->findByName(wrapper, /*maxResults=*/1);
        if (matches.empty()) return;

        const auto& match = matches.front();
        const std::string referencingPath = UriUtils::uriToFilePath(uri);
        const std::string symbolPath = UriUtils::uriToFilePath(match.fileUri);
        if (symbolPath == referencingPath) return;

        const std::string spelling =
            analysis::WorkspaceSymbolIndex::computeImportSpelling(
                symbolPath, referencingPath);
        const int insertLine = doc ? util::findImportInsertLine(doc->content) : 0;
        item.detail = "Auto-import from \"" + spelling + "\"";
        item.additionalTextEdits.push_back(
            utils::buildImportInsertEdit(insertLine, wrapper, spelling));
    }
}
