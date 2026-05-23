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

    // Two classes both expose a `value` method. Searching for Foo.value must
    // not return calls dispatched on a Bar receiver. The pre-AST implementation
    // failed this: it ran a textual whole-word scan for "value" and matched
    // every occurrence in the file.
    harness.addTest("method search is class-scoped", []() {
        const std::string source =
            "class Foo {\n"                                       // line 0
            "    public function value(): int { return 1; }\n"    // line 1
            "}\n"                                                 // line 2
            "class Bar {\n"                                       // line 3
            "    public function value(): int { return 2; }\n"    // line 4
            "}\n"                                                 // line 5
            "Foo f = new Foo();\n"                                // line 6
            "Bar b = new Bar();\n"                                // line 7
            "int x = f.value();\n"                                // line 8
            "int y = b.value();\n";                               // line 9

        auto docMgr = makeDocManager("file:///scoped.mt", source);
        ReferencesHandler handler(docMgr.get());

        // Cursor on Foo's `value` declaration: line 1, inside the `value` token
        // (col 20 = the `v` of `value`).
        auto refs = handler.handleReferences("file:///scoped.mt", {1, 21}, true);

        int fooCallRefs = 0;
        int barCallRefs = 0;
        for (const auto& r : refs) {
            if (r.range.start.line == 8) fooCallRefs++;
            if (r.range.start.line == 9) barCallRefs++;
        }
        require(fooCallRefs >= 1, "expected f.value() call to be included");
        require(barCallRefs == 0, "b.value() must not be matched by Foo.value search");
    });

    // Virtual dispatch means a search on Base.run should also include the
    // overriding Derived.run declaration. mType has no @Override requirement
    // in the test file, so just matching method names is enough.
    harness.addTest("instance method search includes overrides", []() {
        const std::string source =
            "class Base {\n"                                  // line 0
            "    public function run(): void {}\n"            // line 1
            "}\n"                                             // line 2
            "class Derived extends Base {\n"                  // line 3
            "    public function run(): void {}\n"            // line 4
            "}\n"                                             // line 5
            "Base b = new Base();\n"                          // line 6
            "Derived d = new Derived();\n"                    // line 7
            "b.run();\n"                                      // line 8
            "d.run();\n";                                     // line 9

        auto docMgr = makeDocManager("file:///poly.mt", source);
        ReferencesHandler handler(docMgr.get());

        // Cursor on Base.run declaration (line 1, col 21 = inside `run`).
        auto refs = handler.handleReferences("file:///poly.mt", {1, 21}, true);

        bool sawBaseDecl = false;
        bool sawDerivedDecl = false;
        bool sawBCall = false;
        bool sawDCall = false;
        for (const auto& r : refs) {
            if (r.range.start.line == 1) sawBaseDecl = true;
            if (r.range.start.line == 4) sawDerivedDecl = true;
            if (r.range.start.line == 8) sawBCall = true;
            if (r.range.start.line == 9) sawDCall = true;
        }
        require(sawBaseDecl, "Base.run declaration missing");
        require(sawDerivedDecl, "Derived.run override declaration missing");
        require(sawBCall, "b.run() call missing");
        require(sawDCall, "d.run() call missing (polymorphic dispatch coverage)");
    });

    // Cross-file static method: the call site lives in a different open
    // document than the declaration. The handler must walk every open URI.
    harness.addTest("static method references span open documents", []() {
        const std::string defSource =
            "class Util {\n"                                                              // line 0
            "    public static function clamp(int lo): int { return lo; }\n"              // line 1
            "}\n";                                                                        // line 2
        const std::string useSource =
            "int x = Util::clamp(5);\n"
            "int y = Util::clamp(10);\n";

        auto docMgr = std::make_unique<DocumentManager>();
        docMgr->openDocument("file:///def.mt", defSource, 1);
        docMgr->parseDocument("file:///def.mt");
        docMgr->openDocument("file:///use.mt", useSource, 1);
        docMgr->parseDocument("file:///use.mt");

        ReferencesHandler handler(docMgr.get());

        // Cursor on Util's `clamp` declaration (line 1, col 28 = inside `clamp`).
        auto refs = handler.handleReferences("file:///def.mt", {1, 28}, true);

        int useFileHits = 0;
        for (const auto& r : refs) {
            if (r.uri == "file:///use.mt") useFileHits++;
        }
        require(useFileHits >= 2,
            "expected at least 2 Util::clamp calls in use.mt, got " + std::to_string(useFileHits));
    });

    // Cross-file free function: same as above but a top-level function.
    harness.addTest("free function references span open documents", []() {
        const std::string defSource =
            "function helper(): int { return 42; }\n";
        const std::string useSource =
            "int x = helper();\n"
            "print(helper());\n";

        auto docMgr = std::make_unique<DocumentManager>();
        docMgr->openDocument("file:///def.mt", defSource, 1);
        docMgr->parseDocument("file:///def.mt");
        docMgr->openDocument("file:///use.mt", useSource, 1);
        docMgr->parseDocument("file:///use.mt");

        ReferencesHandler handler(docMgr.get());

        // Cursor on `helper` declaration (line 0, column 9).
        auto refs = handler.handleReferences("file:///def.mt", {0, 9}, true);

        int useFileHits = 0;
        for (const auto& r : refs) {
            if (r.uri == "file:///use.mt") useFileHits++;
        }
        require(useFileHits >= 2,
            "expected at least 2 helper() calls in use.mt, got " + std::to_string(useFileHits));
    });

    // MYT-362 — Find References on a chained method whose receiver is itself
    // a method call must type the receiver and pin the search to that
    // class/interface. Before the fix, the chained-call branch fell through
    // to resolveCall's name-only enumeration and over-matched any class
    // declaring a same-named method. Here `Other.withAge` should NOT appear
    // when the cursor is on the chained `.withAge()` whose receiver is
    // typed as Builder.
    harness.addTest("MYT-362 find references on chained interface-method call does not over-match", []() {
        const std::string src =
            "interface Builder {\n"                                              // 0
            "    function withName(string n): Builder;\n"                         // 1
            "    function withAge(int a): Builder;\n"                             // 2
            "}\n"                                                                 // 3
            "class PB implements Builder {\n"                                     // 4
            "    public constructor() {}\n"                                       // 5
            "    public function withName(string n): Builder { return this; }\n"  // 6
            "    public function withAge(int a): Builder { return this; }\n"     // 7
            "}\n"                                                                 // 8
            "class Other {\n"                                                     // 9
            "    public constructor() {}\n"                                       // 10
            "    public function withAge(int a): Other { return this; }\n"        // 11
            "}\n"                                                                 // 12
            "PB pb = new PB();\n"                                                 // 13
            "Other o = new Other();\n"                                            // 14
            "pb.withName(\"a\").withAge(1);\n"                                    // 15 chained, receiver = interface Builder
            "o.withAge(7);\n";                                                    // 16 unrelated Other.withAge call
        auto docMgr = makeDocManager("file:///t.mt", src);
        ReferencesHandler handler(docMgr.get());

        // Cursor on chained .withAge — col 19 (middle of "withAge" at
        // line 15: pb(0-1).(2)w(3)i(4)t(5)h(6)N(7)a(8)m(9)e(10)((11)
        // "(12)a(13)"(14))(15).(16)w(17)i(18)t(19)h(20)A(21)g(22)e(23) ...)
        auto refs = handler.handleReferences("file:///t.mt", {15, 19}, false);

        // The chained `.withAge(1)` at line 15 must be in the results.
        bool foundLine15 = false;
        for (const auto& r : refs) {
            if (r.range.start.line == 15) foundLine15 = true;
        }
        require(foundLine15,
            "chained .withAge() at line 15 should be matched as a Builder.withAge reference");

        // The unrelated `o.withAge(7);` call at line 16 must NOT be among
        // the results, since the chained `.withAge` is typed as Builder.
        for (const auto& r : refs) {
            require(r.range.start.line != 16,
                "Other.withAge() call at line 16 should not be matched by chained Builder.withAge reference search");
        }
    });
}

} // namespace mtype::lsp::test
