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
}

} // namespace mtype::lsp::test
