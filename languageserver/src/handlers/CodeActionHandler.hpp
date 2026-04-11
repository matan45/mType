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
    // ----- Diagnostic-driven dispatch (MYT-35 Phase 5) -----

    // Look at the diagnostic's `data` blob and `code` field, then route
    // to the appropriate per-fix generator. The generator builds a
    // CodeAction whose `diagnostics` field carries the diagnostic that
    // triggered it (so VS Code can attribute the bulb to the right
    // squiggle). Returns an empty vector if no fix applies.
    std::vector<CodeAction> generateFixesForDiagnostic(
        const std::string& uri,
        const Diagnostic& diagnostic
    );

    // Phase 5 fix #2 — typo in identifier. Triggered when the converter
    // attached a `did you mean ...` suggestion via `data["suggestions"]`.
    // Builds a single TextEdit replacing the bad identifier (the
    // diagnostic's range) with the suggested name from the data blob.
    std::vector<CodeAction> generateTypoFixActions(
        const std::string& uri,
        const Diagnostic& diagnostic
    );

    // ----- Diagnostic-agnostic actions (always considered) -----

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
