#pragma once

#include <memory>
#include <unordered_set>
#include <vector>
#include <string>
#include "../utils/LSPTypes.hpp"
#include "../DocumentManager.hpp"

namespace mtype::lsp::analysis
{
    class WorkspaceSymbolIndex;
}

namespace mtype::lsp {

class ProjectConfigProvider;

class CodeActionHandler {
public:
    // MYT-47 — `workspaceIndex` may be null (e.g., in tests). The
    // missing-import fix gracefully no-ops when it is.
    explicit CodeActionHandler(
        DocumentManager* docMgr,
        std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex = nullptr);

    std::vector<CodeAction> handleCodeAction(
        const std::string& uri,
        const Range& range,
        const std::vector<Diagnostic>& diagnostics
    );

    void setProjectConfig(std::shared_ptr<ProjectConfigProvider> config);

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

    // MYT-47 — missing import. Triggered when the diagnostic's data blob
    // identifies an UndefinedException / ClassNotFoundException source.
    // Queries the workspace symbol index for files defining the missing
    // identifier and returns one CodeAction per match (capped to 5).
    std::vector<CodeAction> generateMissingImportFixes(
        const std::string& uri,
        const Diagnostic& diagnostic
    );

    // MYT-360 — primitive type used in a generic argument
    // (e.g. `Predicate<int>`). Triggered when the diagnostic carries
    // `data["exceptionType"] == "PrimitiveInGenericException"`. Rewrites
    // the primitive token (int/float/bool/string) to its boxed class
    // (Int/Float/Bool/String) and inserts the corresponding import when
    // it isn't already present.
    std::vector<CodeAction> generatePrimitiveInGenericFixes(
        const std::string& uri,
        const Diagnostic& diagnostic
    );

    // ----- Diagnostic-agnostic actions (always considered) -----

    // Generate code actions for implementing missing interface methods
    std::vector<CodeAction> getImplementInterfaceActions(
        const std::string& uri,
        int line
    );

    // Refactor: generate getters and setters for the class's fields.
    // Offered whenever the cursor is inside a class with at least one
    // eligible field that lacks an accessor. Emits explicit source text
    // (the IDE-style alternative to the compile-time @Getter/@Setter
    // Lombok pass).
    std::vector<CodeAction> generateGetterSetterActions(
        const std::string& uri,
        int line
    );

    // Refactor: generate a default (no-arg) constructor. Offered when
    // the cursor is inside a class that does not already declare one.
    std::vector<CodeAction> generateDefaultConstructorAction(
        const std::string& uri,
        int line
    );

    DocumentManager* documentManager_;
    std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex_;
    std::shared_ptr<ProjectConfigProvider> projectConfig_;
};

} // namespace mtype::lsp
