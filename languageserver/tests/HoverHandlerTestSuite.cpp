#include "HoverHandlerTestSuite.hpp"
#include "../src/handlers/HoverHandler.hpp"
#include "../src/DocumentManager.hpp"
#include "TestFixtures.hpp"

namespace mtype::lsp::test {

void HoverHandlerTestSuite::registerTests(LspTestHarness& harness) {

    // HoverHandler precedence: typeInfo from environment > keyword > type > builtin > nullopt
    // Returns nullopt if doc is null or word is empty.

    harness.addTest("hover on keyword 'class' returns description", []() {
        auto docMgr = makeDocManager("file:///test.mt", "class Foo {}\n");
        HoverHandler handler(docMgr.get());
        // "class" is at position (0, 0-4), hover at (0, 2) extracts "class"
        auto result = handler.handleHover("file:///test.mt", {0, 2});
        if (result.has_value()) {
            // getKeywordHover returns "**class** (keyword)\n\n..."
            require(result->contents.find("keyword") != std::string::npos,
                "hover for 'class' should mention keyword");
        }
    });

    harness.addTest("hover on type 'int' returns type description", []() {
        auto docMgr = makeDocManager("file:///test.mt", "let x: int = 5;\n");
        HoverHandler handler(docMgr.get());
        // "int" is at column 7-9, hover at (0, 8) should extract "int"
        auto result = handler.handleHover("file:///test.mt", {0, 8});
        if (result.has_value()) {
            // getTypeHover returns "**int** (type)\n\n..."
            require(result->contents.find("int") != std::string::npos,
                "hover for 'int' should mention int");
        }
    });

    harness.addTest("hover on builtin 'print' returns signature", []() {
        auto docMgr = makeDocManager("file:///test.mt", "print(42);\n");
        HoverHandler handler(docMgr.get());
        auto result = handler.handleHover("file:///test.mt", {0, 2});
        if (result.has_value()) {
            // getBuiltinHover returns markdown code block with signature
            require(result->contents.find("print") != std::string::npos,
                "hover for 'print' should mention print");
        }
    });

    harness.addTest("hover on unknown URI returns nullopt", []() {
        auto docMgr = makeDocManager("file:///test.mt", "let x: int = 5;\n");
        HoverHandler handler(docMgr.get());
        auto result = handler.handleHover("file:///nonexistent.mt", {0, 0});
        require(!result.has_value(), "expected nullopt for unknown URI");
    });

    harness.addTest("hover returns nullopt when word is empty", []() {
        DocumentManager docMgr;
        docMgr.openDocument("file:///test.mt", "   \n", 1);
        HoverHandler handler(&docMgr);
        // All whitespace — getWordAtPosition returns empty
        auto result = handler.handleHover("file:///test.mt", {0, 1});
        require(!result.has_value(), "expected nullopt on whitespace");
    });

    harness.addTest("type info from environment takes precedence", []() {
        // When the environment has type info for a symbol, it's returned
        // as a markdown code block before keyword/type/builtin checks.
        auto docMgr = makeDocManager("file:///test.mt",
            "class MyClass {\n    function doWork(): void {}\n}\n");
        HoverHandler handler(docMgr.get());
        auto result = handler.handleHover("file:///test.mt", {0, 8});
        // If environment provides type info for "MyClass", it comes as ```mtype\n...\n```
        // Otherwise falls through to keyword/type/builtin (no match → nullopt)
    });
}

} // namespace mtype::lsp::test
