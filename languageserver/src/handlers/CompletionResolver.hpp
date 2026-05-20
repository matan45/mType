#pragma once

#include "../DocumentManager.hpp"
#include "../utils/LSPTypes.hpp"

namespace mtype::lsp
{
    // Companion for completionItem/resolve. The client echoes the
    // original CompletionItem (with the opaque `data` blob the server
    // stamped during the initial response); we fill in `documentation`
    // derived from the per-document registry without recomputing the
    // whole list. Best-effort — if `data` is missing or the document
    // closed, the item is returned unchanged.
    class CompletionResolver
    {
    public:
        explicit CompletionResolver(DocumentManager* docMgr);

        CompletionItem resolveCompletion(const CompletionItem& item) const;

    private:
        DocumentManager* documentManager_;
    };
}
