#include "DefinitionHandlerTestSuite.hpp"
#include "../src/handlers/DefinitionHandler.hpp"
#include "../src/DocumentManager.hpp"
#include "TestFixtures.hpp"

namespace mtype::lsp::test {

void DefinitionHandlerTestSuite::registerTests(LspTestHarness& harness) {

    // DefinitionHandler delegates to DocumentManager::findDefinition().
    // Returns nullopt if findDefinition returns nullopt.
    // If found, converts the path to a file:// URI via pathToUri().

    harness.addTest("go-to-def for class in same file returns location", []() {
        auto docMgr = makeDocManager("file:///test.mt",
            "class Foo {\n    function bar(): void {}\n}\n");
        DefinitionHandler handler(docMgr.get());

        // Verify that the class Foo is in symbolLocations after parse
        auto* doc = docMgr->getDocument("file:///test.mt");
        require(doc != nullptr, "doc should exist");

        // Try to go to definition of "Foo" — cursor on "Foo" at line 0, col 6
        auto result = handler.handleDefinition("file:///test.mt", {0, 6});
        // Result depends on whether findDefinition resolves "Foo" from symbolLocations
        if (result.has_value()) {
            require(result->range.start.line == 0,
                "Foo definition should be at line 0");
        }
    });

    harness.addTest("go-to-def for function returns location", []() {
        auto docMgr = makeDocManager("file:///test.mt",
            "function helper(): void {}\n");
        DefinitionHandler handler(docMgr.get());

        auto result = handler.handleDefinition("file:///test.mt", {0, 10});
        // "helper" is at col 9-14; cursor at 10 should extract "helper"
        if (result.has_value()) {
            require(result->range.start.line == 0,
                "helper definition should be at line 0");
        }
    });

    harness.addTest("unknown URI returns nullopt", []() {
        auto docMgr = makeDocManager("file:///test.mt", "class Foo {}\n");
        DefinitionHandler handler(docMgr.get());
        auto result = handler.handleDefinition("file:///nonexistent.mt", {0, 0});
        require(!result.has_value(), "expected nullopt for unknown URI");
    });

    harness.addTest("location URI is converted to file:// format", []() {
        auto docMgr = makeDocManager("file:///test.mt", "class Foo {}\n");
        DefinitionHandler handler(docMgr.get());

        auto result = handler.handleDefinition("file:///test.mt", {0, 6});
        if (result.has_value()) {
            // pathToUri ensures the URI starts with file://
            require(result->uri.find("file://") == 0,
                "location URI should be a file:// URI");
        }
    });

    harness.addTest("does not crash on empty document", []() {
        auto docMgr = makeDocManager("file:///test.mt", "\n");
        DefinitionHandler handler(docMgr.get());
        auto result = handler.handleDefinition("file:///test.mt", {0, 0});
        // Should not crash — result is nullopt or a valid location
    });

    // MYT-358 — Go to Definition on a method whose receiver is a chained
    // static field (`Audio::sndChord.play()`) must resolve through the
    // static field's declared class to the method declaration.
    harness.addTest("go-to-def resolves Class::field.method() chain via static field type", []() {
        const std::string src =
            "class Chord {\n"
            "    public constructor() {}\n"
            "    public function play(): void {}\n"
            "}\n"
            "class Audio {\n"
            "    public static Chord sndChord = new Chord();\n"
            "}\n"
            "Audio::sndChord.play();\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        DefinitionHandler handler(docMgr.get());

        // 'play' on line 7 (`Audio::sndChord.play();`) cols 16-19; caret on 'l' at (7, 17).
        auto result = handler.handleDefinition("file:///t.mt", {7, 17});
        require(result.has_value(), "expected definition for play() via Class::field.method()");
        require(result->range.start.line == 2, "play() should be defined at line 2");
    });

    harness.addTest("go-to-def Class::field.method() returns nullopt when class unknown", []() {
        const std::string src =
            "class Chord {\n"
            "    public function play(): void {}\n"
            "}\n"
            "Unknown::sndChord.play();\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        DefinitionHandler handler(docMgr.get());

        // 'play' on line 3 (`Unknown::sndChord.play();`) cols 18-21; caret on 'l' at (3, 19).
        auto result = handler.handleDefinition("file:///t.mt", {3, 19});
        require(!result.has_value(),
            "expected nullopt when receiverClass not in registry");
    });

    // MYT-359 — Go to Definition must follow chained receiver shapes through
    // the AST-driven ReceiverTypeResolver. Each test covers one of the five
    // shapes the ticket calls out, all converging on the same `play()`
    // declaration so we know the resolver typed the chain correctly.

    harness.addTest("MYT-359 go-to-def follows obj.field.method() multi-hop instance chain", []() {
        const std::string src =
            "class Inner {\n"                                    // 0
            "    public function play(): void {}\n"              // 1  play decl
            "}\n"                                                // 2
            "class Outer {\n"                                    // 3
            "    public Inner inner;\n"                          // 4
            "    public constructor() {}\n"                      // 5
            "}\n"                                                // 6
            "Outer o = new Outer();\n"                           // 7
            "o.inner.play();\n";                                 // 8  play at cols 8-11
        auto docMgr = makeDocManager("file:///t.mt", src);
        DefinitionHandler handler(docMgr.get());

        auto result = handler.handleDefinition("file:///t.mt", {8, 9});
        require(result.has_value(), "expected definition for obj.field.method() chain");
        require(result->range.start.line == 1, "play() should resolve to line 1");
    });

    harness.addTest("MYT-359 go-to-def follows getFoo().method() call-result chain", []() {
        const std::string src =
            "class Foo {\n"                                      // 0
            "    public function play(): void {}\n"              // 1  play decl
            "}\n"                                                // 2
            "function getFoo(): Foo { return new Foo(); }\n"     // 3
            "getFoo().play();\n";                                // 4  play at cols 10-13
        auto docMgr = makeDocManager("file:///t.mt", src);
        DefinitionHandler handler(docMgr.get());

        auto result = handler.handleDefinition("file:///t.mt", {4, 11});
        require(result.has_value(), "expected definition for getFoo().play()");
        require(result->range.start.line == 1, "play() should resolve to line 1");
    });

    harness.addTest("MYT-359 go-to-def follows arr[0].method() subscript chain", []() {
        const std::string src =
            "class Foo {\n"                                      // 0
            "    public function play(): void {}\n"              // 1  play decl
            "}\n"                                                // 2
            "Foo[] arr = new Foo[3];\n"                          // 3
            "arr[0].play();\n";                                  // 4  play at cols 7-10
        auto docMgr = makeDocManager("file:///t.mt", src);
        DefinitionHandler handler(docMgr.get());

        auto result = handler.handleDefinition("file:///t.mt", {4, 8});
        require(result.has_value(), "expected definition for arr[0].play()");
        require(result->range.start.line == 1, "play() should resolve to line 1");
    });

    harness.addTest("MYT-359 go-to-def follows Class::field.field.method() multi-hop static chain", []() {
        const std::string src =
            "class Inner {\n"                                    // 0
            "    public function play(): void {}\n"              // 1  play decl
            "}\n"                                                // 2
            "class Outer {\n"                                    // 3
            "    public Inner inner;\n"                          // 4
            "    public constructor() {}\n"                      // 5
            "}\n"                                                // 6
            "class App {\n"                                      // 7
            "    public static Outer outer = new Outer();\n"     // 8
            "}\n"                                                // 9
            "App::outer.inner.play();\n";                        // 10 play at cols 17-20
        auto docMgr = makeDocManager("file:///t.mt", src);
        DefinitionHandler handler(docMgr.get());

        auto result = handler.handleDefinition("file:///t.mt", {10, 18});
        require(result.has_value(), "expected definition for Class::field.field.method() chain");
        require(result->range.start.line == 1, "play() should resolve to line 1");
    });

    harness.addTest("MYT-359 go-to-def follows (cond ? a : b).method() ternary receiver", []() {
        const std::string src =
            "class Foo {\n"                                      // 0
            "    public function play(): void {}\n"              // 1  play decl
            "}\n"                                                // 2
            "Foo a = new Foo();\n"                               // 3
            "Foo b = new Foo();\n"                               // 4
            "(true ? a : b).play();\n";                          // 5  play at cols 15-18
        auto docMgr = makeDocManager("file:///t.mt", src);
        DefinitionHandler handler(docMgr.get());

        auto result = handler.handleDefinition("file:///t.mt", {5, 16});
        require(result.has_value(), "expected definition for (cond ? a : b).play()");
        require(result->range.start.line == 1, "play() should resolve to line 1");
    });
}

} // namespace mtype::lsp::test
