#include "SymbolRegistrationVisitorTestSuite.hpp"
#include "../src/DocumentManager.hpp"
#include "TestFixtures.hpp"

namespace mtype::lsp::test {

void SymbolRegistrationVisitorTestSuite::registerTests(LspTestHarness& harness) {

    harness.addTest("class registered in symbolLocations after parse", []() {
        auto docMgr = makeDocManager("file:///test.mt", "class MyClass {}\n");
        auto* doc = docMgr->getDocument("file:///test.mt");
        require(doc != nullptr, "document should exist");

        bool found = doc->symbolLocations.count("MyClass") > 0;
        require(found, "expected 'MyClass' in symbolLocations");
    });

    harness.addTest("function registered in symbolLocations after parse", []() {
        auto docMgr = makeDocManager("file:///test.mt", "function myFunc(): void {}\n");
        auto* doc = docMgr->getDocument("file:///test.mt");
        require(doc != nullptr, "document should exist");

        bool found = doc->symbolLocations.count("myFunc") > 0;
        require(found, "expected 'myFunc' in symbolLocations");
    });

    harness.addTest("interface registered in symbolLocations after parse", []() {
        auto docMgr = makeDocManager("file:///test.mt",
            "interface Printable {\n    function print(): void;\n}\n");
        auto* doc = docMgr->getDocument("file:///test.mt");
        require(doc != nullptr, "document should exist");

        bool found = doc->symbolLocations.count("Printable") > 0;
        require(found, "expected 'Printable' in symbolLocations");
    });

    harness.addTest("multiple symbols all registered", []() {
        auto docMgr = makeDocManager("file:///test.mt",
            "class Alpha {}\nclass Beta {}\nfunction gamma(): void {}\n");
        auto* doc = docMgr->getDocument("file:///test.mt");
        require(doc != nullptr, "document should exist");

        require(doc->symbolLocations.count("Alpha") > 0, "expected Alpha");
        require(doc->symbolLocations.count("Beta") > 0, "expected Beta");
        require(doc->symbolLocations.count("gamma") > 0, "expected gamma");
    });

    harness.addTest("symbol location has correct line", []() {
        auto docMgr = makeDocManager("file:///test.mt",
            "class First {}\nclass Second {}\n");
        auto* doc = docMgr->getDocument("file:///test.mt");
        require(doc != nullptr, "document should exist");

        if (doc->symbolLocations.count("First")) {
            // Line should be 0 (0-based for LSP)
            require(doc->symbolLocations.at("First").line == 0,
                "First should be at line 0");
        }
        if (doc->symbolLocations.count("Second")) {
            require(doc->symbolLocations.at("Second").line == 1,
                "Second should be at line 1");
        }
    });
}

} // namespace mtype::lsp::test
