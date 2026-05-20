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
    // Count references to a class or interface in the current document.
    int countReferences(const std::string& symbolName, const Document* doc, int declarationLine);

    // Find all usages of a symbol in the current document.
    std::vector<Location> findUsages(const std::string& symbolName, const Document* doc, int declarationLine);

    DocumentManager* documentManager_;
};

} // namespace mtype::lsp
