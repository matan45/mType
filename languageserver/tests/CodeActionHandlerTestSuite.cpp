#include "CodeActionHandlerTestSuite.hpp"

#include "../src/handlers/CodeActionHandler.hpp"
#include "../src/DocumentManager.hpp"
#include "../src/analysis/WorkspaceSymbolIndex.hpp"
#include "../src/utils/LSPTypes.hpp"
#include "TestFixtures.hpp"

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

    // ---------------------------------------------------------------
    // Test 3: No workspace index — graceful degradation
    // ---------------------------------------------------------------
    harness.addTest("nullptr workspace index: no import fix returned", []() {
        auto docMgr = makeDocManager("file:///test/main.mt", "let x = new Unknown();\n");
        CodeActionHandler handler(docMgr.get()); // nullptr workspace index

        auto diag = makeDiagnostic({{0, 12}, {0, 19}},
            "Undefined class 'Unknown'", "ClassNotFoundException");
        auto actions = handler.handleCodeAction("file:///test/main.mt", diag.range, {diag});

        for (const auto& action : actions) {
            require(action.title.find("Add import") == std::string::npos,
                "should not generate import fix without workspace index");
        }
    });

    // ---------------------------------------------------------------
    // Test 4: Typo fix from suggestions
    // ---------------------------------------------------------------
    harness.addTest("typo fix: diagnostic with suggestions produces fix action", []() {
        auto docMgr = makeDocManager("file:///test/typo.mt",
            "class Greeter {}\nlet g = new Greete();\n");
        CodeActionHandler handler(docMgr.get());

        auto diag = makeDiagnosticWithSuggestions(
            {{1, 12}, {1, 18}}, "Undefined class 'Greete'", {"Greeter"});

        auto actions = handler.handleCodeAction("file:///test/typo.mt", diag.range, {diag});

        bool foundTypoFix = false;
        for (const auto& action : actions) {
            if (action.edit.has_value()) {
                for (const auto& [uri, edits] : action.edit->changes) {
                    for (const auto& edit : edits) {
                        if (edit.newText.find("Greeter") != std::string::npos) {
                            foundTypoFix = true;
                        }
                    }
                }
            }
        }
        require(foundTypoFix,
            "expected a typo fix action replacing 'Greete' with 'Greeter'");
    });

    // ---------------------------------------------------------------
    // Test 5: Multiple candidates from workspace index
    // ---------------------------------------------------------------
    harness.addTest("multiple workspace matches produce multiple import actions", []() {
        auto docMgr = makeDocManager("file:///test/main.mt", "let h = new Helper();\n");

        auto wsIndex = std::make_shared<analysis::WorkspaceSymbolIndex>();
        wsIndex->reindexFile("file:///a/Helper.mt", "class Helper {}\n");
        wsIndex->reindexFile("file:///b/Helper.mt", "class Helper {}\n");

        CodeActionHandler handler(docMgr.get(), wsIndex);
        auto diag = makeDiagnostic({{0, 12}, {0, 18}},
            "Undefined class 'Helper'", "ClassNotFoundException");

        auto actions = handler.handleCodeAction("file:///test/main.mt", diag.range, {diag});

        int importActionCount = 0;
        for (const auto& action : actions) {
            if (action.title.find("Add import") != std::string::npos
                && action.title.find("Helper") != std::string::npos) {
                ++importActionCount;
            }
        }
        require(importActionCount >= 2,
            "expected at least 2 import actions for 2 workspace matches, got "
            + std::to_string(importActionCount));
    });

    // ---------------------------------------------------------------
    // Test 6: Implement interface action
    // ---------------------------------------------------------------
    harness.addTest("implement interface: generates stub methods", []() {
        auto docMgr = makeDocManager("file:///test/impl.mt",
            "interface Printable {\n    function toString(): string;\n}\n"
            "class Widget implements Printable {\n}\n");
        CodeActionHandler handler(docMgr.get());

        // Request code actions at the class declaration line
        Range range{{3, 0}, {3, 40}};
        auto actions = handler.handleCodeAction("file:///test/impl.mt", range, {});

        bool foundImplement = false;
        for (const auto& action : actions) {
            if (action.title.find("Implement") != std::string::npos
                || action.title.find("implement") != std::string::npos) {
                foundImplement = true;
                if (action.edit.has_value()) {
                    for (const auto& [uri, edits] : action.edit->changes) {
                        for (const auto& edit : edits) {
                            require(edit.newText.find("toString") != std::string::npos,
                                "stub should contain 'toString' method");
                        }
                    }
                }
                break;
            }
        }
        require(foundImplement,
            "expected an 'Implement interface' code action");
    });

    // ---------------------------------------------------------------
    // Test 7: No implement action when no implements clause
    // ---------------------------------------------------------------
    harness.addTest("no implement action for class without implements", []() {
        auto docMgr = makeDocManager("file:///test/plain.mt", "class Plain {}\n");
        CodeActionHandler handler(docMgr.get());

        Range range{{0, 0}, {0, 14}};
        auto actions = handler.handleCodeAction("file:///test/plain.mt", range, {});

        for (const auto& action : actions) {
            require(action.title.find("Implement") == std::string::npos,
                "should not offer implement action for class without 'implements'");
        }
    });
}

} // namespace mtype::lsp::test
