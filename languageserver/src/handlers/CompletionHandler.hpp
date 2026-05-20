#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "../utils/LSPTypes.hpp"
#include "../DocumentManager.hpp"
#include "AutoImportCompletionProvider.hpp"
#include "CompletionResolver.hpp"
#include "MemberCompletionProvider.hpp"
#include "PathCompletionHandler.hpp"

namespace environment
{
    class Environment;
}

namespace mtype::lsp::analysis
{
    class WorkspaceSymbolIndex;
}

namespace mtype::lsp
{
    class ProjectConfigProvider;

    // Top-level completion dispatcher. Routes per-trigger contexts
    // (`.`/`::`, `@`, `extends`/`implements`, generic identifier) to a
    // small set of sub-providers and assembles the final ranked item
    // list. The four sub-providers (`PathCompletionHandler`,
    // `MemberCompletionProvider`, `AutoImportCompletionProvider`,
    // `CompletionResolver`) own their feature-specific state.
    class CompletionHandler
    {
    public:
        // `workspaceIndex` may be null — the auto-import branches degrade
        // to no-ops. Mirrors CodeActionHandler's constructor shape.
        explicit CompletionHandler(
            DocumentManager* docMgr,
            std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex = nullptr);

        std::vector<CompletionItem> handleCompletion(
            const std::string& uri,
            const Position& position);

        // Forwarded to PathCompletionHandler so import-string completions
        // can enumerate `@pkg` aliases.
        void setProjectConfig(std::shared_ptr<ProjectConfigProvider> config);

        CompletionItem resolveCompletion(const CompletionItem& item) const;

    private:
        // Per-context dispatch. Each returns `nullopt` to fall through to
        // the next context; a present value (possibly empty) means "this
        // context took the request and produced its definitive answer."
        std::optional<std::vector<CompletionItem>> tryMemberCompletions(
            const std::string& uri,
            const Position& position,
            const std::string& trimmedBefore,
            const std::string& typedPrefix,
            bool followedByOpenParen);
        std::optional<std::vector<CompletionItem>> tryAnnotationCompletions(
            const std::string& uri,
            const Position& position,
            const std::string& textBeforeCursor) const;
        std::optional<std::vector<CompletionItem>> tryInheritanceCompletions(
            const std::string& uri,
            const Position& position,
            const std::string& textBeforeCursor) const;
        std::vector<CompletionItem> buildGeneralCompletions(
            const std::string& uri,
            const Position& position,
            const std::string& typedPrefix,
            bool followedByOpenParen) const;

        // Primitive keywords + capitalized wrapper classes. The wrapper
        // auto-import enrichment is delegated to AutoImportCompletionProvider.
        // (Pure-static catalogues — keywords/builtins/collections — live
        // as free functions in internal::CompletionHelpers.)
        std::vector<CompletionItem> getTypeCompletions(const std::string& uri) const;
        std::vector<CompletionItem> getAnnotationCompletions(const std::string& uri) const;

        // Environment-driven identifier completions.
        std::vector<CompletionItem> getIdentifierCompletions(const std::string& uri) const;
        std::vector<CompletionItem> getClassCompletions(const std::string& uri) const;
        std::vector<CompletionItem> getInterfaceCompletions(const std::string& uri) const;

        // Per-kind sub-helpers for getIdentifierCompletions (kept under
        // the 50-line method cap). `seen` is shared across them so the
        // same symbol isn't surfaced twice across overlapping pools.
        void addVariableItems(std::vector<CompletionItem>& items,
                              const environment::Environment& env,
                              std::unordered_set<std::string>& seen) const;
        void addFunctionItems(std::vector<CompletionItem>& items,
                              const environment::Environment& env,
                              const std::string& uri,
                              std::unordered_set<std::string>& seen) const;
        void addClassItems(std::vector<CompletionItem>& items,
                           const environment::Environment& env,
                           std::unordered_set<std::string>& seen) const;
        void addInterfaceItems(std::vector<CompletionItem>& items,
                               const environment::Environment& env,
                               std::unordered_set<std::string>& seen) const;

        std::string getLineAtPosition(const std::string& content,
                                      const Position& position) const;

        // Rustc-style Levenshtein filter. Items whose label prefix-matches
        // `typedPrefix` are always kept; others drop if
        // `levenshtein(typedPrefix, label) > max(1, typedPrefix.size()/3)`.
        // A no-op when `typedPrefix` is empty.
        void applyFuzzyFilter(std::vector<CompletionItem>& items,
                              const std::string& typedPrefix) const;

        // Used in every dispatch branch: when the cursor is already
        // immediately followed by `(`, snippet bodies on callable items
        // would produce `name(${1:p})$0()` against the existing paren.
        // Strip the body back to a bare identifier in that case.
        // MUST run before applyReplacementEdit — otherwise the textEdit
        // captures the snippet form.
        void stripCallSnippets(std::vector<CompletionItem>& items,
                               bool followedByOpenParen) const;

        DocumentManager* documentManager_;
        std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex_;
        std::unique_ptr<PathCompletionHandler> pathCompletionHandler_;
        std::unique_ptr<MemberCompletionProvider> memberCompletionProvider_;
        std::unique_ptr<AutoImportCompletionProvider> autoImportProvider_;
        std::unique_ptr<CompletionResolver> completionResolver_;
    };
}
