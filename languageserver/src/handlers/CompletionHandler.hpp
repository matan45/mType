#pragma once

#include <vector>
#include <string>
#include "../utils/LSPTypes.hpp"
#include "../DocumentManager.hpp"
#include "PathCompletionHandler.hpp"
#include <memory>

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
    std::vector<CompletionItem> getClassCompletions(const std::string& uri) const;
    std::vector<CompletionItem> getInterfaceCompletions(const std::string& uri) const;
    std::string getLineAtPosition(const std::string& content, const Position& position) const;

    DocumentManager* documentManager_;
    std::unique_ptr<PathCompletionHandler> pathCompletionHandler_;
};

} // namespace mtype::lsp
