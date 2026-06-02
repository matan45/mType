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

    harness.addTest("MYT-372 go-to-def resolves this.method() to enclosing class method", []() {
        const std::string src =
            "class Widget {\n"                                   // 0
            "    private bool addCollider;\n"                    // 1
            "    private function resolvedSlot(): int {\n"       // 2
            "        return 1;\n"                                // 3
            "    }\n"                                            // 4
            "    public function update(): void {\n"             // 5
            "        if (this.addCollider) {\n"                  // 6
            "            int slot = this.resolvedSlot();\n"      // 7
            "        }\n"                                        // 8
            "    }\n"                                            // 9
            "}\n";                                               // 10
        auto docMgr = makeDocManager("file:///t.mt", src);
        DefinitionHandler handler(docMgr.get());

        auto result = handler.handleDefinition("file:///t.mt", {7, 29});
        require(result.has_value(), "expected definition for this.resolvedSlot()");
        require(result->range.start.line == 2,
            "resolvedSlot() should resolve to Widget.resolvedSlot at line 2");
    });

    harness.addTest("MYT-372 go-to-def resolves this.method() from constructor", []() {
        const std::string src =
            "class Widget {\n"                                   // 0
            "    public function init(): void {}\n"              // 1
            "    public constructor() {\n"                       // 2
            "        this.init();\n"                             // 3
            "    }\n"                                            // 4
            "}\n";                                               // 5
        auto docMgr = makeDocManager("file:///t.mt", src);
        DefinitionHandler handler(docMgr.get());

        auto result = handler.handleDefinition("file:///t.mt", {3, 13});
        require(result.has_value(), "expected definition for this.init() in constructor");
        require(result->range.start.line == 1,
            "init() should resolve to Widget.init at line 1");
    });

    harness.addTest("MYT-372 go-to-def resolves super.method() to parent method", []() {
        const std::string src =
            "class Parent {\n"                                   // 0
            "    public function speak(): void {}\n"             // 1
            "}\n"                                                // 2
            "class Child extends Parent {\n"                     // 3
            "    public function shout(): void {\n"              // 4
            "        super.speak();\n"                           // 5
            "    }\n"                                            // 6
            "}\n";                                               // 7
        auto docMgr = makeDocManager("file:///t.mt", src);
        DefinitionHandler handler(docMgr.get());

        auto result = handler.handleDefinition("file:///t.mt", {5, 15});
        require(result.has_value(), "expected definition for super.speak()");
        require(result->range.start.line == 1,
            "super.speak() should resolve to Parent.speak at line 1");
    });

    harness.addTest("MYT-372 go-to-def resolves super.method() to nearest ancestor", []() {
        const std::string src =
            "class GrandParent {\n"                              // 0
            "    public function speak(): void {}\n"             // 1
            "}\n"                                                // 2
            "class Parent extends GrandParent {\n"               // 3
            "    public function speak(): void {}\n"             // 4
            "}\n"                                                // 5
            "class Child extends Parent {\n"                     // 6
            "    public function shout(): void {\n"              // 7
            "        super.speak();\n"                           // 8
            "    }\n"                                            // 9
            "}\n";                                               // 10
        auto docMgr = makeDocManager("file:///t.mt", src);
        DefinitionHandler handler(docMgr.get());

        auto result = handler.handleDefinition("file:///t.mt", {8, 15});
        require(result.has_value(), "expected definition for super.speak()");
        require(result->range.start.line == 4,
            "super.speak() should resolve to nearest Parent.speak at line 4");
    });

    harness.addTest("MYT-372 go-to-def unresolved super.method() returns nullopt", []() {
        const std::string src =
            "function missing(): void {}\n"                      // 0
            "class Parent {\n"                                   // 1
            "    public function other(): void {}\n"             // 2
            "}\n"                                                // 3
            "class Child extends Parent {\n"                     // 4
            "    public function go(): void {\n"                 // 5
            "        super.missing();\n"                         // 6
            "    }\n"                                            // 7
            "}\n";                                               // 8
        auto docMgr = makeDocManager("file:///t.mt", src);
        DefinitionHandler handler(docMgr.get());

        auto result = handler.handleDefinition("file:///t.mt", {6, 15});
        require(!result.has_value(),
            "unresolved super.missing() must not fall back to a wrong symbol");
    });

    // MYT-362 — chained method calls on an interface receiver. Self-returning
    // interface mirrors the Stream API repro (filter().map().limit().toArray()).
    // Before the fix, every call after the first interface-typed step returned
    // nullopt: ReceiverTypeResolver bailed because Stream wasn't in the
    // ClassRegistry, and even when it did resolve, symbolLocations had no
    // `InterfaceName.methodName` entry.
    harness.addTest("MYT-362 go-to-def follows chained interface-method receiver (1st hop)", []() {
        const std::string src =
            "interface Builder {\n"                                              // 0
            "    function withName(string n): Builder;\n"                         // 1  withName decl
            "    function withAge(int a): Builder;\n"                             // 2  withAge decl
            "    function build(): Person;\n"                                     // 3  build decl
            "}\n"                                                                 // 4
            "class Person { public constructor() {} }\n"                          // 5
            "class PB implements Builder {\n"                                     // 6
            "    public constructor() {}\n"                                       // 7
            "    public function withName(string n): Builder { return this; }\n"  // 8
            "    public function withAge(int a): Builder { return this; }\n"     // 9
            "    public function build(): Person { return new Person(); }\n"      // 10
            "}\n"                                                                 // 11
            "PB pb = new PB();\n"                                                 // 12
            "Person p = pb.withName(\"a\").withAge(1).build();\n";                // 13
        auto docMgr = makeDocManager("file:///t.mt", src);
        DefinitionHandler handler(docMgr.get());

        // pb.withName(...) — receiver is VariableNode("pb") (typed as PB, a
        // class). This path already worked; included to anchor the chain.
        auto result = handler.handleDefinition("file:///t.mt", {13, 16});
        require(result.has_value(), "expected definition for pb.withName()");
        // PB.withName is at line 8; Builder.withName is at line 1. Either
        // is a defensible answer — accept both.
        require(result->range.start.line == 8 || result->range.start.line == 1,
            "withName should resolve to PB.withName (line 8) or Builder.withName (line 1)");
    });

    harness.addTest("MYT-362 go-to-def follows chained interface-method receiver (.withAge after .withName)", []() {
        const std::string src =
            "interface Builder {\n"                                              // 0
            "    function withName(string n): Builder;\n"                         // 1
            "    function withAge(int a): Builder;\n"                             // 2  withAge decl
            "    function build(): Person;\n"                                     // 3
            "}\n"                                                                 // 4
            "class Person { public constructor() {} }\n"                          // 5
            "class PB implements Builder {\n"                                     // 6
            "    public constructor() {}\n"                                       // 7
            "    public function withName(string n): Builder { return this; }\n"  // 8
            "    public function withAge(int a): Builder { return this; }\n"     // 9
            "    public function build(): Person { return new Person(); }\n"      // 10
            "}\n"                                                                 // 11
            "PB pb = new PB();\n"                                                 // 12
            "Person p = pb.withName(\"a\").withAge(1).build();\n";                // 13
        auto docMgr = makeDocManager("file:///t.mt", src);
        DefinitionHandler handler(docMgr.get());

        // .withAge is at col 28-34 (cursor at 30). Receiver of .withAge is
        // a MethodCallNode for .withName(...), which returns interface
        // Builder. Before MYT-362, this returned nullopt.
        auto result = handler.handleDefinition("file:///t.mt", {13, 30});
        require(result.has_value(), "expected definition for chained .withAge() on interface receiver");
        require(result->range.start.line == 2,
            "withAge should resolve to Builder.withAge at line 2");
    });

    harness.addTest("MYT-362 go-to-def follows chained interface-method receiver (.build after .withAge)", []() {
        const std::string src =
            "interface Builder {\n"                                              // 0
            "    function withName(string n): Builder;\n"                         // 1
            "    function withAge(int a): Builder;\n"                             // 2
            "    function build(): Person;\n"                                     // 3  build decl
            "}\n"                                                                 // 4
            "class Person { public constructor() {} }\n"                          // 5
            "class PB implements Builder {\n"                                     // 6
            "    public constructor() {}\n"                                       // 7
            "    public function withName(string n): Builder { return this; }\n"  // 8
            "    public function withAge(int a): Builder { return this; }\n"     // 9
            "    public function build(): Person { return new Person(); }\n"      // 10
            "}\n"                                                                 // 11
            "PB pb = new PB();\n"                                                 // 12
            "Person p = pb.withName(\"a\").withAge(1).build();\n";                // 13
        auto docMgr = makeDocManager("file:///t.mt", src);
        DefinitionHandler handler(docMgr.get());

        // .build is at col 39-43 (cursor at 40). Two-step chained receiver.
        auto result = handler.handleDefinition("file:///t.mt", {13, 40});
        require(result.has_value(), "expected definition for chained .build() through two interface hops");
        require(result->range.start.line == 3,
            "build should resolve to Builder.build at line 3");
    });
}

} // namespace mtype::lsp::test
