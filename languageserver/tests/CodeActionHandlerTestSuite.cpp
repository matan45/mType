#include "CodeActionHandlerTestSuite.hpp"

#include "../src/handlers/CodeActionHandler.hpp"
#include "../src/DocumentManager.hpp"
#include "../src/analysis/WorkspaceSymbolIndex.hpp"
#include "../src/utils/LSPTypes.hpp"

#include <string>

namespace mtype::lsp::test {

void CodeActionHandlerTestSuite::registerTests(LspTestHarness& harness) {

    // ---------------------------------------------------------------
    // Test 1: Missing import generates a quick-fix CodeAction
    // ---------------------------------------------------------------
    harness.addTest("missing import generates quick-fix with import edit", []() {
        DocumentManager docMgr;
        const std::string mainUri = "file:///test/main.mt";
        const std::string libUri  = "file:///test/lib/Util.mt";

        const std::string mainSource = "let u = new Util();\n";
        docMgr.openDocument(mainUri, mainSource, 1);
        docMgr.parseDocument(mainUri);

        // Seed workspace index with Util in another file
        auto wsIndex = std::make_shared<analysis::WorkspaceSymbolIndex>();
        wsIndex->reindexFile(libUri, "class Util {\n}\n");

        CodeActionHandler handler(&docMgr, wsIndex);

        // Build a diagnostic that mimics what the LSP would produce for
        // an undefined class reference — data.exceptionType = "ClassNotFoundException"
        Diagnostic diag;
        diag.range = {{0, 12}, {0, 16}};  // range covering "Util"
        diag.severity = 1;
        diag.message = "Undefined class 'Util'";
        diag.data = nlohmann::json{
            {"exceptionType", "ClassNotFoundException"}
        };

        auto actions = handler.handleCodeAction(mainUri, diag.range, {diag});

        // Expect at least one quick-fix action for the missing import
        bool foundImportFix = false;
        for (const auto& action : actions) {
            if (action.title.find("Add import") != std::string::npos
                && action.title.find("Util") != std::string::npos) {
                foundImportFix = true;
                require(action.kind.has_value() && *action.kind == "quickfix",
                    "expected kind 'quickfix'");
                require(action.edit.has_value(),
                    "expected CodeAction to have a WorkspaceEdit");
                // The edit should target the main file
                require(action.edit->changes.count(mainUri) == 1,
                    "expected edit changes keyed by main URI");
                require(!action.edit->changes.at(mainUri).empty(),
                    "expected at least one TextEdit");
                require(action.edit->changes.at(mainUri)[0].newText.find("import") != std::string::npos,
                    "expected TextEdit newText to contain an import statement");
                break;
            }
        }
        require(foundImportFix,
            "expected a 'Add import' CodeAction for missing class 'Util'");
    });

    // ---------------------------------------------------------------
    // Test 2: No self-import — symbol in same file doesn't generate action
    // ---------------------------------------------------------------
    harness.addTest("no self-import for symbol defined in same file", []() {
        DocumentManager docMgr;
        const std::string uri = "file:///test/self.mt";

        const std::string source = "class Foo {}\nlet f = new Foo();\n";
        docMgr.openDocument(uri, source, 1);
        docMgr.parseDocument(uri);

        // Seed workspace index with Foo — but from the same file
        auto wsIndex = std::make_shared<analysis::WorkspaceSymbolIndex>();
        wsIndex->reindexFile(uri, source);

        CodeActionHandler handler(&docMgr, wsIndex);

        Diagnostic diag;
        diag.range = {{1, 12}, {1, 15}};
        diag.severity = 1;
        diag.message = "Undefined class 'Foo'";
        diag.data = nlohmann::json{
            {"exceptionType", "ClassNotFoundException"}
        };

        auto actions = handler.handleCodeAction(uri, diag.range, {diag});

        // No import action should be generated because Foo is in the same file
        for (const auto& action : actions) {
            require(action.title.find("Add import") == std::string::npos
                    || action.title.find("Foo") == std::string::npos,
                "should not generate self-import action for 'Foo' from same file");
        }
    });
}

} // namespace mtype::lsp::test
