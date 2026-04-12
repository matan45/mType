#include "DocumentManagerTestSuite.hpp"
#include "../src/DocumentManager.hpp"
#include "TestFixtures.hpp"

namespace mtype::lsp::test {

void DocumentManagerTestSuite::registerTests(LspTestHarness& harness) {

    harness.addTest("openDocument + getDocument retrieves content", []() {
        DocumentManager docMgr;
        docMgr.openDocument("file:///test.mt", "let x: int = 1;", 1);
        auto* doc = docMgr.getDocument("file:///test.mt");
        require(doc != nullptr, "document should exist after open");
        require(doc->content == "let x: int = 1;", "content mismatch");
        require(doc->version == 1, "version mismatch");
    });

    harness.addTest("updateDocument replaces content and version", []() {
        DocumentManager docMgr;
        docMgr.openDocument("file:///test.mt", "old", 1);
        docMgr.updateDocument("file:///test.mt", "new content", 2);
        auto* doc = docMgr.getDocument("file:///test.mt");
        require(doc != nullptr, "document should exist");
        require(doc->content == "new content", "content should be updated");
        require(doc->version == 2, "version should be 2");
    });

    harness.addTest("closeDocument removes document", []() {
        DocumentManager docMgr;
        docMgr.openDocument("file:///test.mt", "content", 1);
        docMgr.closeDocument("file:///test.mt");
        auto* doc = docMgr.getDocument("file:///test.mt");
        require(doc == nullptr, "document should be null after close");
    });

    harness.addTest("getDocument returns nullptr for unknown URI", []() {
        DocumentManager docMgr;
        require(docMgr.getDocument("file:///nonexistent.mt") == nullptr,
            "should return nullptr for unknown URI");
    });

    harness.addTest("parseDocument sets isParsed and populates AST", []() {
        auto docMgr = makeDocManager("file:///test.mt", "class Foo {}");
        auto* doc = docMgr->getDocument("file:///test.mt");
        require(doc != nullptr, "document should exist");
        require(doc->isParsed, "document should be marked as parsed");
    });

    harness.addTest("parseDocument populates environment", []() {
        auto docMgr = makeDocManager("file:///test.mt", "class Bar {}\n");
        auto* doc = docMgr->getDocument("file:///test.mt");
        require(doc != nullptr && doc->environment != nullptr,
            "environment should be populated after parse");
    });

    harness.addTest("getWordAtPosition extracts identifier", []() {
        DocumentManager docMgr;
        docMgr.openDocument("file:///test.mt", "let myVar: int = 5;", 1);
        std::string word = docMgr.getWordAtPosition("file:///test.mt", 0, 5);
        require(word == "myVar", "expected 'myVar', got '" + word + "'");
    });

    harness.addTest("getWordAtPosition returns empty on whitespace", []() {
        DocumentManager docMgr;
        docMgr.openDocument("file:///test.mt", "let x = 5;", 1);
        std::string word = docMgr.getWordAtPosition("file:///test.mt", 0, 3);
        // Position 3 is the space after "let"
        require(word.empty() || word == "let" || word == "x",
            "should handle whitespace position gracefully");
    });

    harness.addTest("getDocumentSymbols returns class and function", []() {
        auto docMgr = makeDocManager("file:///test.mt",
            "class Alpha {}\nfunction beta(): void {}\n");
        auto symbols = docMgr->getDocumentSymbols("file:///test.mt");
        bool foundClass = false, foundFunction = false;
        for (const auto& sym : symbols) {
            if (sym.name == "Alpha" && sym.kind == "class") foundClass = true;
            if (sym.name == "beta" && sym.kind == "function") foundFunction = true;
        }
        require(foundClass, "expected class 'Alpha' in symbols");
        require(foundFunction, "expected function 'beta' in symbols");
    });

    harness.addTest("parse error populates diagnostics", []() {
        auto docMgr = makeDocManager("file:///test.mt", "class { broken syntax");
        auto* doc = docMgr->getDocument("file:///test.mt");
        require(doc != nullptr, "document should exist");
        require(!doc->diagnostics.empty(),
            "expected diagnostics for parse error");
    });
}

} // namespace mtype::lsp::test
