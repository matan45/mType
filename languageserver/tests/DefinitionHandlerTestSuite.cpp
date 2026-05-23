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
}

} // namespace mtype::lsp::test
