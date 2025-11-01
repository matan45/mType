#pragma once

#include <vector>
#include <string>
#include "../utils/LSPTypes.hpp"
#include "../DocumentManager.hpp"

namespace mtype::lsp {

class CompletionHandler {
public:
    explicit CompletionHandler(DocumentManager* docMgr);

    std::vector<CompletionItem> handleCompletion(
        const std::string& uri,
        const Position& position
    );

private:
    std::vector<CompletionItem> getKeywordCompletions() const;
    std::vector<CompletionItem> getTypeCompletions() const;
    std::vector<CompletionItem> getBuiltinCompletions() const;
    std::vector<CompletionItem> getVariableCompletions(const std::string& uri, int line) const;
    std::vector<CompletionItem> getCollectionCompletions() const;

    DocumentManager* documentManager_;
};

} // namespace mtype::lsp
