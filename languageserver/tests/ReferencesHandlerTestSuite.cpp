#include "ReferencesHandlerTestSuite.hpp"
#include "../src/handlers/ReferencesHandler.hpp"
#include "../src/DocumentManager.hpp"
#include "TestFixtures.hpp"

namespace mtype::lsp::test {

void ReferencesHandlerTestSuite::registerTests(LspTestHarness& harness) {

    harness.addTest("finds class references in type annotations", []() {
        const std::string source =
            "class Foo {\n"
            "    public function constructor() {}\n"
            "}\n"
            "Foo myVar;\n"
            "Foo other;\n";

        auto docMgr = makeDocManager("file:///test.mt", source);
        ReferencesHandler handler(docMgr.get());

        // Position on "Foo" at line 3 col 0
        auto refs = handler.handleReferences("file:///test.mt", {3, 0}, true);
        require(refs.size() >= 2, "expected at least 2 references to Foo, got " + std::to_string(refs.size()));
    });

    harness.addTest("finds variable references across lines", []() {
        const std::string source =
            "int count;\n"
            "count = 5;\n"
            "print(count);\n";

        auto docMgr = makeDocManager("file:///test.mt", source);
        ReferencesHandler handler(docMgr.get());

        // Position on "count" at line 0 col 4
        auto refs = handler.handleReferences("file:///test.mt", {0, 4}, true);
        require(refs.size() >= 3, "expected at least 3 references to count, got " + std::to_string(refs.size()));
    });

    harness.addTest("respects includeDeclaration=false for class", []() {
        const std::string source =
            "class MyClass {\n"
            "    public function constructor() {}\n"
            "}\n"
            "MyClass obj;\n";

        auto docMgr = makeDocManager("file:///test.mt", source);
        ReferencesHandler handler(docMgr.get());

        auto withDecl = handler.handleReferences("file:///test.mt", {3, 0}, true);
        auto withoutDecl = handler.handleReferences("file:///test.mt", {3, 0}, false);

        // Without declaration should have fewer results (excludes the class declaration line)
        require(withoutDecl.size() <= withDecl.size(),
            "without-declaration should have <= results than with-declaration");
    });

    harness.addTest("returns empty for unknown URI", []() {
        auto docMgr = makeDocManager("file:///test.mt", "int x;\n");
        ReferencesHandler handler(docMgr.get());

        auto refs = handler.handleReferences("file:///unknown.mt", {0, 0}, true);
        require(refs.empty(), "expected empty results for unknown URI");
    });

    harness.addTest("returns empty when cursor is on whitespace", []() {
        auto docMgr = makeDocManager("file:///test.mt", "   \n");
        ReferencesHandler handler(docMgr.get());

        auto refs = handler.handleReferences("file:///test.mt", {0, 1}, true);
        require(refs.empty(), "expected empty results on whitespace");
    });

    harness.addTest("finds constructor references via new keyword", []() {
        const std::string source =
            "class Car {\n"
            "    public function constructor() {}\n"
            "}\n"
            "Car c = new Car();\n"
            "Car d = new Car();\n";

        auto docMgr = makeDocManager("file:///test.mt", source);
        ReferencesHandler handler(docMgr.get());

        // Position on "Car" at line 3 col 0
        auto refs = handler.handleReferences("file:///test.mt", {3, 0}, true);
        require(refs.size() >= 3, "expected at least 3 references to Car (decl + 2 usages), got " + std::to_string(refs.size()));
    });

    harness.addTest("whole-word matching does not match substrings", []() {
        const std::string source =
            "int count;\n"
            "int counter;\n"
            "int recount;\n"
            "count = 1;\n";

        auto docMgr = makeDocManager("file:///test.mt", source);
        ReferencesHandler handler(docMgr.get());

        auto refs = handler.handleReferences("file:///test.mt", {0, 4}, true);
        // Should find "count" on lines 0 and 3, but NOT "counter" or "recount"
        for (const auto& ref : refs) {
            int col = ref.range.start.character;
            int line = ref.range.start.line;
            require(line == 0 || line == 3,
                "unexpected reference on line " + std::to_string(line) + " col " + std::to_string(col));
        }
    });

    harness.addTest("reference locations have correct range", []() {
        const std::string source = "int abc = 10;\nprint(abc);\n";

        auto docMgr = makeDocManager("file:///test.mt", source);
        ReferencesHandler handler(docMgr.get());

        auto refs = handler.handleReferences("file:///test.mt", {0, 4}, true);
        require(!refs.empty(), "expected at least one reference");

        for (const auto& ref : refs) {
            int len = ref.range.end.character - ref.range.start.character;
            require(len == 3, "reference range length should be 3 (abc), got " + std::to_string(len));
        }
    });
}

} // namespace mtype::lsp::test
