#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "../utils/LSPTypes.hpp"
#include "../DocumentManager.hpp"
#include "PathCompletionHandler.hpp"

namespace mtype::lsp::analysis
{
    class WorkspaceSymbolIndex;
}

namespace mtype::lsp {

class CompletionHandler {
public:
    // MYT-51 — `workspaceIndex` may be null (default arg keeps legacy
    // call sites compiling). The auto-import branch is a no-op when
    // it is. Mirrors CodeActionHandler's constructor shape.
    explicit CompletionHandler(
        DocumentManager* docMgr,
        std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex = nullptr);

    std::vector<CompletionItem> handleCompletion(
        const std::string& uri,
        const Position& position
    );

    // Lazy enrichment for completionItem/resolve. The client echoes the
    // original CompletionItem (including the opaque `data` blob the
    // server stamped during the initial response); this fills in
    // documentation derived from the registry without recomputing the
    // whole list.
    CompletionItem resolveCompletion(const CompletionItem& item) const;

private:
    std::vector<CompletionItem> getKeywordCompletions() const;
    // Lowercase primitive keywords + the capitalized wrapper classes
    // (Int / Float / Bool / String / Promise). The wrappers are real
    // classes living in lib/primitives/*.mt — when the workspace index
    // can resolve them they're emitted with an auto-import attached so
    // accepting the completion also inserts `import { Int } from "..."`.
    std::vector<CompletionItem> getTypeCompletions(const std::string& uri) const;
    std::vector<CompletionItem> getBuiltinCompletions() const;
    std::vector<CompletionItem> getCollectionCompletions() const;
    // Annotation completions sourced from the per-document
    // AnnotationRegistry (populated with built-ins via
    // BuiltInAnnotations + any user-declared annotations). Triggered
    // when the cursor sits right after `@` (optionally followed by a
    // partial identifier).
    std::vector<CompletionItem> getAnnotationCompletions(const std::string& uri) const;

    // MYT-51 — unified identifier enumeration. Walks the environment's
    // scope chain + class/interface/function registries through
    // diagnostics::IdentifierEnumerator in one pass, tagging each
    // name with the right CompletionItemKind by re-querying the
    // registries on the way out.
    std::vector<CompletionItem> getIdentifierCompletions(const std::string& uri) const;

    // Context-narrowed variants used after `extends` / `implements`.
    std::vector<CompletionItem> getClassCompletions(const std::string& uri) const;
    std::vector<CompletionItem> getInterfaceCompletions(const std::string& uri) const;

    std::vector<CompletionItem> getMemberCompletions(
        const std::string& uri,
        const std::string& objectName,
        int line,
        bool isStaticAccess) const;

    // MYT-51 — auto-import completion items. Queries the workspace
    // symbol index for names matching `typedPrefix` that are defined
    // in other files and attaches an additionalTextEdits import line.
    // No-op when `workspaceIndex_` is null or `typedPrefix` is empty.
    std::vector<CompletionItem> getAutoImportCompletions(
        const std::string& uri,
        const std::string& typedPrefix,
        const std::vector<CompletionItem>& existingItems) const;

    std::string getLineAtPosition(const std::string& content, const Position& position) const;

    // MYT-51 — apply the rustc-style Levenshtein filter in place.
    // Items whose label prefix-matches `typedPrefix` are always kept;
    // other items are dropped if `levenshtein(typedPrefix, label)`
    // exceeds `max(1, typedPrefix.size() / 3)`. A no-op if
    // `typedPrefix` is empty.
    void applyFuzzyFilter(
        std::vector<CompletionItem>& items,
        const std::string& typedPrefix) const;

    DocumentManager* documentManager_;
    std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex_;
    std::unique_ptr<PathCompletionHandler> pathCompletionHandler_;

    // Per-document member completion cache. `getMemberCompletions` is
    // called on every keystroke after `.` / `::`, and the underlying
    // inheritance walk + dedup is the same answer until the document
    // changes. Keyed by `uri + access + "@" + typeName`; entries are
    // invalidated by document version. Guarded by a mutex because the
    // LSP runtime is free to dispatch requests on any thread.
    struct MemberCacheEntry {
        int docVersion;
        std::vector<CompletionItem> items;
    };
    mutable std::mutex memberCacheMutex_;
    mutable std::unordered_map<std::string, MemberCacheEntry> memberCache_;
};

} // namespace mtype::lsp
