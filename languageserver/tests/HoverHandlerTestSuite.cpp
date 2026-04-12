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
        // The word "class" may be extracted as the keyword, or
        // getWordAtPosition may return "class" which then hits the keyword map.
        // However, if the environment resolves typeInfo for "class" first,
        // that takes precedence. Either way, we should get a result.
        require(result.has_value(), "expected hover result for keyword 'class'");
        require(result->contents.find("class") != std::string::npos,
            "hover contents should mention 'class'");
    });

    harness.addTest("hover on type 'int' returns type description", []() {
        auto docMgr = makeDocManager("file:///test.mt", "let x: int = 5;\n");
        HoverHandler handler(docMgr.get());
        auto result = handler.handleHover("file:///test.mt", {0, 8});
        require(result.has_value(), "expected hover result for type 'int'");
        require(result->contents.find("int") != std::string::npos,
            "hover for 'int' should mention int");
    });

    harness.addTest("hover on builtin 'print' returns signature", []() {
        auto docMgr = makeDocManager("file:///test.mt", "print(42);\n");
        HoverHandler handler(docMgr.get());
        auto result = handler.handleHover("file:///test.mt", {0, 2});
        require(result.has_value(), "expected hover result for builtin 'print'");
        require(result->contents.find("print") != std::string::npos,
            "hover for 'print' should mention print");
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

    harness.addTest("hover contents are non-empty for known symbols", []() {
        auto docMgr = makeDocManager("file:///test.mt", "return;\n");
        HoverHandler handler(docMgr.get());
        // "return" is in the keyword map
        auto result = handler.handleHover("file:///test.mt", {0, 3});
        require(result.has_value(), "expected hover for keyword 'return'");
        require(!result->contents.empty(), "hover contents should not be empty");
    });
}

} // namespace mtype::lsp::test
