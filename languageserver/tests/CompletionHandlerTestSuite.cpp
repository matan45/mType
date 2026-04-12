#include "CompletionHandlerTestSuite.hpp"

#include "../src/handlers/CompletionHandler.hpp"
#include "../src/DocumentManager.hpp"
#include "../src/analysis/WorkspaceSymbolIndex.hpp"
#include "../src/utils/LSPTypes.hpp"

#include <algorithm>
#include <string>

namespace mtype::lsp::test {

namespace {

bool hasItemWithLabel(const std::vector<CompletionItem>& items, const std::string& label) {
    return std::any_of(items.begin(), items.end(),
        [&](const CompletionItem& item) { return item.label == label; });
}

bool hasItemWithKind(const std::vector<CompletionItem>& items, const std::string& label, int kind) {
    return std::any_of(items.begin(), items.end(),
        [&](const CompletionItem& item) { return item.label == label && item.kind == kind; });
}

} // anonymous namespace

void CompletionHandlerTestSuite::registerTests(LspTestHarness& harness) {

    // ---------------------------------------------------------------
    // Test 1: Keyword completions on empty prefix
    // ---------------------------------------------------------------
    harness.addTest("keyword completions appear on empty line", []() {
        DocumentManager docMgr;
        const std::string uri = "file:///test/keywords.mt";
        // Single empty line — cursor at (0, 0) with no typed prefix
        docMgr.openDocument(uri, "\n", 1);
        docMgr.parseDocument(uri);

        CompletionHandler handler(&docMgr);
        auto items = handler.handleCompletion(uri, {0, 0});

        require(!items.empty(), "expected non-empty completion list");
        require(hasItemWithLabel(items, "class"),
            "expected 'class' keyword in completions");
        require(hasItemWithLabel(items, "function"),
            "expected 'function' keyword in completions");
        require(hasItemWithLabel(items, "return"),
            "expected 'return' keyword in completions");
        require(hasItemWithLabel(items, "if"),
            "expected 'if' keyword in completions");
    });

    // ---------------------------------------------------------------
    // Test 2: Multiple document-defined symbols all visible in completions
    // ---------------------------------------------------------------
    harness.addTest("multiple classes and functions from document visible", []() {
        DocumentManager docMgr;
        const std::string uri = "file:///test/multi.mt";
        const std::string source = R"(
class Alpha {
    function doAlpha(): void {}
}

class Beta {
    function doBeta(): void {}
}

function helperFn(): void {}

)";
        docMgr.openDocument(uri, source, 1);
        docMgr.parseDocument(uri);

        CompletionHandler handler(&docMgr);
        // Cursor at the blank line at end
        auto items = handler.handleCompletion(uri, {10, 0});

        require(hasItemWithLabel(items, "Alpha"),
            "expected 'Alpha' class in completions");
        require(hasItemWithLabel(items, "Beta"),
            "expected 'Beta' class in completions");
        require(hasItemWithLabel(items, "helperFn"),
            "expected 'helperFn' function in completions");
    });

    // ---------------------------------------------------------------
    // Test 3: Classification — names tagged with correct CompletionItemKind
    // ---------------------------------------------------------------
    harness.addTest("class and function tagged with correct CompletionItemKind", []() {
        DocumentManager docMgr;
        const std::string uri = "file:///test/classify.mt";
        const std::string source = R"(
class Greeter {
    function greet(): void {}
}

function topLevelFn(): void {}

)";
        docMgr.openDocument(uri, source, 1);
        docMgr.parseDocument(uri);

        CompletionHandler handler(&docMgr);
        // Cursor at the blank line at end — general completions
        auto items = handler.handleCompletion(uri, {6, 0});

        // CompletionItemKind::Class = 7, Function = 3
        require(hasItemWithKind(items, "Greeter", 7),
            "expected 'Greeter' tagged as Class (kind 7)");
        require(hasItemWithKind(items, "topLevelFn", 3),
            "expected 'topLevelFn' tagged as Function (kind 3)");
    });

    // ---------------------------------------------------------------
    // Test 4: Fuzzy filter — 'prnt' keeps 'print' (distance 1, budget 1)
    // ---------------------------------------------------------------
    harness.addTest("fuzzy filter: prnt keeps print", []() {
        DocumentManager docMgr;
        const std::string uri = "file:///test/fuzzy_keep.mt";
        // Type "prnt" — builtin "print" should survive the fuzzy filter
        // (levenshtein("prnt","print") = 1, budget = max(1, 4/3) = 1).
        const std::string source = "prnt";
        docMgr.openDocument(uri, source, 1);
        docMgr.parseDocument(uri);

        CompletionHandler handler(&docMgr);
        auto items = handler.handleCompletion(uri, {0, 4});

        require(hasItemWithLabel(items, "print"),
            "expected 'print' to survive fuzzy filter for prefix 'prnt'");
    });

    // ---------------------------------------------------------------
    // Test 5: Fuzzy filter — 'xyz' drops unrelated items
    // ---------------------------------------------------------------
    harness.addTest("fuzzy filter: xyz drops unrelated completions", []() {
        DocumentManager docMgr;
        const std::string uri = "file:///test/fuzzy_drop.mt";
        const std::string source = "xyz";
        docMgr.openDocument(uri, source, 1);
        docMgr.parseDocument(uri);

        CompletionHandler handler(&docMgr);
        auto items = handler.handleCompletion(uri, {0, 3});

        // With prefix "xyz" (length 3), budget = max(1, 3/3) = 1.
        // No keyword/builtin is within distance 1 of "xyz".
        require(!hasItemWithLabel(items, "class"),
            "expected 'class' to be filtered out for prefix 'xyz'");
        require(!hasItemWithLabel(items, "print"),
            "expected 'print' to be filtered out for prefix 'xyz'");
    });

    // ---------------------------------------------------------------
    // Test 6: Auto-import via WorkspaceSymbolIndex
    // ---------------------------------------------------------------
    harness.addTest("auto-import: workspace class surfaces with import edit", []() {
        DocumentManager docMgr;
        const std::string mainUri = "file:///test/main.mt";
        const std::string libUri  = "file:///test/lib/Helper.mt";

        // Main file: user is typing "Helper"
        const std::string mainSource = "Helper";
        docMgr.openDocument(mainUri, mainSource, 1);
        docMgr.parseDocument(mainUri);

        // Seed WorkspaceSymbolIndex with a class from another file
        auto wsIndex = std::make_shared<analysis::WorkspaceSymbolIndex>();
        const std::string libSource = "class Helper {\n}\n";
        wsIndex->reindexFile(libUri, libSource);

        CompletionHandler handler(&docMgr, wsIndex);
        auto items = handler.handleCompletion(mainUri, {0, 6});

        // Find the auto-import item
        bool foundAutoImport = false;
        for (const auto& item : items) {
            if (item.label == "Helper" && item.detail.has_value()
                && item.detail->find("Auto-import from") == 0) {
                foundAutoImport = true;
                require(!item.additionalTextEdits.empty(),
                    "auto-import item should have additionalTextEdits");
                require(item.additionalTextEdits[0].newText.find("import") != std::string::npos,
                    "additionalTextEdits should contain an import statement");
                break;
            }
        }
        require(foundAutoImport,
            "expected auto-import completion item for 'Helper' from workspace index");
    });
}

} // namespace mtype::lsp::test
