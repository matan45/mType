#pragma once

#include <vector>
#include <string>
#include "../utils/LSPTypes.hpp"
#include "../DocumentManager.hpp"

namespace mtype::lsp {

class CodeActionHandler {
public:
    explicit CodeActionHandler(DocumentManager* docMgr);

    std::vector<CodeAction> handleCodeAction(
        const std::string& uri,
        const Range& range,
        const std::vector<Diagnostic>& diagnostics
    );

private:
    // Generate code actions for implementing missing interface methods
    std::vector<CodeAction> getImplementInterfaceActions(
        const std::string& uri,
        int line
    );

    // Helper to get all methods required by an interface
    std::vector<std::string> getRequiredMethods(
        const std::string& interfaceName,
        const Document* doc
    );

    // Helper to check if a class already has a method
    bool classHasMethod(
        const std::string& className,
        const std::string& methodName,
        const Document* doc
    );

    DocumentManager* documentManager_;
};

} // namespace mtype::lsp
