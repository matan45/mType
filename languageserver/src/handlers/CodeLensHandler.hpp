#pragma once

#include <vector>
#include <string>
#include "../utils/LSPTypes.hpp"
#include "../DocumentManager.hpp"

namespace mtype::lsp {

class CodeLensHandler {
public:
    explicit CodeLensHandler(DocumentManager* docMgr);

    std::vector<CodeLens> handleCodeLens(const std::string& uri);

private:
    // Count references to a class or interface
    int countReferences(const std::string& symbolName, const std::string& uri);

    // Find all usages of a symbol in the document
    std::vector<Location> findUsages(const std::string& symbolName, const Document* doc);

    DocumentManager* documentManager_;
};

} // namespace mtype::lsp
