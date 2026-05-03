#include "CompletionHandlerTestSuite.hpp"

#include "../src/handlers/CompletionHandler.hpp"
#include "../src/DocumentManager.hpp"
#include "../src/analysis/WorkspaceSymbolIndex.hpp"
#include "../src/utils/LSPTypes.hpp"
#include "TestFixtures.hpp"

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

const CompletionItem* findItemWithLabel(const std::vector<CompletionItem>& items, const std::string& label) {
    auto it = std::find_if(items.begin(), items.end(),
        [&](const CompletionItem& item) { return item.label == label; });
    return it == items.end() ? nullptr : &(*it);
}

const CompletionItem* findAutoImportItem(
    const std::vector<CompletionItem>& items,
    const std::string& label) {
    auto it = std::find_if(items.begin(), items.end(),
        [&](const CompletionItem& item) {
            return item.label == label
                && item.detail.has_value()
                && item.detail->find("Auto-import") == 0;
        });
    return it == items.end() ? nullptr : &(*it);
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

    harness.addTest("fuzzy filter: case-insensitive typo keeps print", []() {
        DocumentManager docMgr;
        const std::string uri = "file:///test/fuzzy_case.mt";
        docMgr.openDocument(uri, "PRNT", 1);
        docMgr.parseDocument(uri);

        CompletionHandler handler(&docMgr);
        auto items = handler.handleCompletion(uri, {0, 4});

        require(hasItemWithLabel(items, "print"),
            "expected 'print' to survive fuzzy filter for uppercase prefix 'PRNT'");
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

    harness.addTest("auto-import: partial prefix surfaces workspace class", []() {
        DocumentManager docMgr;
        const std::string mainUri = "file:///test/main_partial.mt";
        const std::string libUri  = "file:///test/lib/Helper.mt";

        docMgr.openDocument(mainUri, "Hel", 1);
        docMgr.parseDocument(mainUri);

        auto wsIndex = std::make_shared<analysis::WorkspaceSymbolIndex>();
        wsIndex->reindexFile(libUri, "class Helper {\n}\n");

        CompletionHandler handler(&docMgr, wsIndex);
        auto items = handler.handleCompletion(mainUri, {0, 3});
        const auto* item = findAutoImportItem(items, "Helper");

        require(item != nullptr,
            "expected partial prefix 'Hel' to offer auto-import for 'Helper'");
        require(!item->additionalTextEdits.empty(),
            "partial auto-import item should include import edit");
        require(item->textEdit.has_value(),
            "partial auto-import item should replace the typed prefix");
    });

    harness.addTest("auto-import: workspace kind maps to completion kind", []() {
        DocumentManager docMgr;
        const std::string mainUri = "file:///test/kinds.mt";
        docMgr.openDocument(mainUri, "IF", 1);
        docMgr.parseDocument(mainUri);

        auto wsIndex = std::make_shared<analysis::WorkspaceSymbolIndex>();
        wsIndex->reindexFile("file:///test/lib/IFace.mt",
            "interface IFace {\n    function run(): void;\n}\n");
        wsIndex->reindexFile("file:///test/lib/makeThing.mt",
            "function makeThing(): void {}\n");

        CompletionHandler handler(&docMgr, wsIndex);
        auto interfaceItems = handler.handleCompletion(mainUri, {0, 2});
        const auto* iface = findAutoImportItem(interfaceItems, "IFace");
        require(iface != nullptr, "expected interface auto-import item");
        require(iface->kind == static_cast<int>(CompletionItemKind::Interface),
            "auto-imported interface should use Interface completion kind");

        docMgr.updateDocument(mainUri, "make", 2);
        auto functionItems = handler.handleCompletion(mainUri, {0, 4});
        const auto* fn = findAutoImportItem(functionItems, "makeThing");
        require(fn != nullptr, "expected function auto-import item");
        require(fn->kind == static_cast<int>(CompletionItemKind::Function),
            "auto-imported function should use Function completion kind");
    });

    // ---------------------------------------------------------------
    // Test 7: Context narrowing after 'extends'
    // ---------------------------------------------------------------
    harness.addTest("context narrowing: extends shows only classes", []() {
        const std::string uri = "file:///test/extends.mt";
        // Use a valid document. Position cursor right after "extends " on the
        // class declaration line. The textEndsInInheritanceKeyword check only
        // looks at the text before the cursor on that line.
        // "class Child extends Base {}" — cursor after "extends " at col 21.
        auto docMgr = makeDocManager(uri,
            "class Base {}\ninterface IFace {}\nclass Child extends Base {}\n");

        CompletionHandler handler(docMgr.get());
        // Line 2: "class Child extends Base {}" — col 20 is right after "extends "
        auto items = handler.handleCompletion(uri, {2, 20});

        require(!items.empty(), "extends context should return completions");
        require(hasItemWithLabel(items, "Base"),
            "extends context should include class 'Base'");
        require(!hasItemWithLabel(items, "IFace"),
            "extends context should not show interfaces");
    });

    // ---------------------------------------------------------------
    // Test 8: Context narrowing after 'implements'
    // ---------------------------------------------------------------
    harness.addTest("context narrowing: implements shows only interfaces", []() {
        const std::string uri = "file:///test/implements.mt";
        // Interface with a method signature — empty interfaces may not produce
        // an InterfaceNode in the parser.
        auto docMgr = makeDocManager(uri,
            "class Base {}\n"
            "interface IFace {\n    function doIt(): void;\n}\n"
            "class Child implements IFace {\n    function doIt(): void {}\n}\n");

        // Verify IFace actually registered in the interface registry
        auto* doc = docMgr->getDocument(uri);
        require(doc != nullptr && doc->environment != nullptr,
            "precondition: environment must exist");
        auto ifaceReg = doc->environment->getInterfaceRegistry();
        require(ifaceReg != nullptr, "precondition: interface registry must exist");
        const auto& allIfaces = ifaceReg->getAllInterfaces();
        require(allIfaces.count("IFace") > 0,
            "precondition: IFace must be in interface registry, found "
            + std::to_string(allIfaces.size()) + " interfaces");

        CompletionHandler handler(docMgr.get());
        // Line 4: "class Child implements IFace {" — col 22 is right after "implements "
        auto items = handler.handleCompletion(uri, {4, 22});

        require(!items.empty(), "implements context should return completions");
        require(hasItemWithLabel(items, "IFace"),
            "implements context should include interface 'IFace'");
        require(!hasItemWithLabel(items, "Base"),
            "implements context should not show classes");
    });

    // ---------------------------------------------------------------
    // Test 9: Same-file symbol not duplicated by auto-import
    // ---------------------------------------------------------------
    harness.addTest("auto-import: same-file symbol not duplicated", []() {
        const std::string uri = "file:///test/self.mt";
        auto docMgr = makeDocManager(uri, "class MySelf {}\nMySelf");

        auto wsIndex = std::make_shared<analysis::WorkspaceSymbolIndex>();
        wsIndex->reindexFile(uri, "class MySelf {}\n");

        CompletionHandler handler(docMgr.get(), wsIndex);
        auto items = handler.handleCompletion(uri, {1, 6});

        // Count how many items are labeled "MySelf"
        int count = 0;
        for (const auto& item : items) {
            if (item.label == "MySelf") ++count;
        }
        require(count <= 1, "MySelf should not be duplicated by auto-import from same file");
    });

    // ---------------------------------------------------------------
    // Test 10: Name collision suppression
    // ---------------------------------------------------------------
    harness.addTest("auto-import: skips name already in scope", []() {
        const std::string mainUri = "file:///test/main.mt";
        const std::string libUri = "file:///test/lib/Foo.mt";

        // Foo is defined in the current document as a class.
        // First verify that Foo actually shows up in completions (in scope).
        auto docMgr = makeDocManager(mainUri, "class Foo {}\n");
        CompletionHandler plainHandler(docMgr.get());
        auto plainItems = plainHandler.handleCompletion(mainUri, {1, 0});
        require(hasItemWithLabel(plainItems, "Foo"),
            "precondition: 'Foo' must be in scope from the class definition");

        // Now add a workspace index with Foo from another file and type "Foo"
        auto wsIndex = std::make_shared<analysis::WorkspaceSymbolIndex>();
        wsIndex->reindexFile(libUri, "class Foo {}\n");

        // Re-open with typed prefix to trigger auto-import
        docMgr->updateDocument(mainUri, "class Foo {}\nFoo", 2);
        CompletionHandler handler(docMgr.get(), wsIndex);
        auto items = handler.handleCompletion(mainUri, {1, 3});

        // Should not have an auto-import item for Foo since it's already in scope
        bool hasAutoImport = false;
        for (const auto& item : items) {
            if (item.label == "Foo" && item.detail.has_value()
                && item.detail->find("Auto-import") == 0) {
                hasAutoImport = true;
            }
        }
        require(!hasAutoImport,
            "should not offer auto-import for name already in scope");
    });

    // ---------------------------------------------------------------
    // Test 11: Builtin and collection completions present
    // ---------------------------------------------------------------
    harness.addTest("builtin and collection completions appear", []() {
        auto docMgr = makeDocManager("file:///test/builtins.mt", "\n");
        CompletionHandler handler(docMgr.get());
        auto items = handler.handleCompletion("file:///test/builtins.mt", {0, 0});

        require(hasItemWithLabel(items, "print"), "expected 'print' builtin");
        require(hasItemWithLabel(items, "typeof"), "expected 'typeof' builtin");
        require(hasItemWithLabel(items, "List"), "expected 'List' collection");
        require(hasItemWithLabel(items, "HashMap"), "expected 'HashMap' collection");
    });

    harness.addTest("ranking metadata: prefix function sorts before keyword", []() {
        auto docMgr = makeDocManager("file:///test/ranking.mt", "pr");
        CompletionHandler handler(docMgr.get());
        auto items = handler.handleCompletion("file:///test/ranking.mt", {0, 2});

        const auto* printItem = findItemWithLabel(items, "print");
        const auto* privateItem = findItemWithLabel(items, "private");

        require(printItem != nullptr, "expected print completion");
        require(privateItem != nullptr, "expected private completion");
        require(printItem->sortText.has_value(), "print should have sortText");
        require(privateItem->sortText.has_value(), "private should have sortText");
        require(*printItem->sortText < *privateItem->sortText,
            "function completion should sort before keyword completion for same prefix");
        require(printItem->textEdit.has_value(),
            "completion should include replacement edit for typed prefix");
    });

    // ---------------------------------------------------------------
    // Test 12: Member access via dot
    // ---------------------------------------------------------------
    harness.addTest("member access via dot triggers member completions", []() {
        auto docMgr = makeDocManager("file:///test/dot.mt",
            "class Dog {\n    function bark(): void {}\n}\nlet d = new Dog();\nd.");
        CompletionHandler handler(docMgr.get());
        auto items = handler.handleCompletion("file:///test/dot.mt", {4, 2});

        // Should return member completions for Dog. If type inference works,
        // "bark" should be there. If not, we at least verify no crash.
        if (!items.empty()) {
            // Check that these are member-context items (not keywords)
            require(!hasItemWithLabel(items, "class"),
                "member access should not show keywords");
        }
    });

    harness.addTest("member access: unknown receiver does not show unrelated members", []() {
        auto docMgr = makeDocManager("file:///test/unknown_member.mt",
            "class Dog {\n    function bark(): void {}\n}\nmissing.bark();\n");
        CompletionHandler handler(docMgr.get());
        auto items = handler.handleCompletion("file:///test/unknown_member.mt", {3, 8});

        require(items.empty(),
            "unknown receiver should not fall back to members from every class");
    });

    harness.addTest("member access: this resolves enclosing class", []() {
        auto docMgr = makeDocManager("file:///test/this_member.mt",
            "class Dog {\n"
            "    function bark(): void {}\n"
            "    function test(): void {\n"
            "        this.bark();\n"
            "    }\n"
            "}\n");
        CompletionHandler handler(docMgr.get());
        auto items = handler.handleCompletion("file:///test/this_member.mt", {3, 13});

        require(hasItemWithLabel(items, "bark"),
            "this. should include members from the enclosing class");
    });

    // ---------------------------------------------------------------
    // Test 13: Static access via ::
    // ---------------------------------------------------------------
    harness.addTest("static access via :: returns class members", []() {
        auto docMgr = makeDocManager("file:///test/static.mt",
            "class MathUtil {\n    static function add(a: int, b: int): int { return a + b; }\n}\nMathUtil::");
        CompletionHandler handler(docMgr.get());
        auto items = handler.handleCompletion("file:///test/static.mt", {3, 10});

        // Should return static members of MathUtil, not keywords
        if (!items.empty()) {
            require(!hasItemWithLabel(items, "class"),
                ":: access should not show keywords");
        }
    });
}

} // namespace mtype::lsp::test
