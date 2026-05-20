#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../DocumentManager.hpp"
#include "../utils/LSPTypes.hpp"

namespace mtype::lsp
{
    // Member-access completions (`obj.method`, `Class::staticField`).
    // Owns the per-document result cache because the dropdown fires on
    // every keystroke after the trigger and the inheritance walk is the
    // same answer until the document version changes.
    //
    // NOT thread-safe: the LSP request loop in MTypeLanguageServer::run
    // is single-threaded; the cache never sees concurrent access. If
    // dispatch ever goes parallel, wrap reads/writes in a mutex.
    class MemberCompletionProvider
    {
    public:
        explicit MemberCompletionProvider(DocumentManager* docMgr);

        std::vector<CompletionItem> getMemberCompletions(
            const std::string& uri,
            const std::string& objectName,
            int line,
            bool isStaticAccess);

    private:
        struct MemberCacheEntry
        {
            int docVersion;
            std::vector<CompletionItem> items;
        };

        DocumentManager* documentManager_;
        std::unordered_map<std::string, MemberCacheEntry> memberCache_;
    };
}
