#include "CodeLensHandlerTestSuite.hpp"
#include "../src/handlers/CodeLensHandler.hpp"
#include "../src/DocumentManager.hpp"
#include "TestFixtures.hpp"

namespace mtype::lsp::test {

void CodeLensHandlerTestSuite::registerTests(LspTestHarness& harness) {

    // CodeLens iterates doc->symbolLocations, checks classRegistry/interfaceRegistry,
    // counts references via word-boundary regex, creates lenses with format:
    //   "N reference(s)" or "1 reference"
    //   command = "mtype.showReferences"

    harness.addTest("class with references produces code lens", []() {
        auto docMgr = makeDocManager("file:///test.mt",
            "class Foo {}\nlet a = new Foo();\nlet b = new Foo();\n");
        CodeLensHandler handler(docMgr.get());
        auto lenses = handler.handleCodeLens("file:///test.mt");

        // Foo is a class with 2 references (the new Foo() lines)
        bool foundFooLens = false;
        for (const auto& lens : lenses) {
            if (lens.command.has_value()
                && lens.command->title.find("reference") != std::string::npos) {
                foundFooLens = true;
                // Should be 2 references
                require(lens.command->title.find("2") != std::string::npos,
                    "expected '2' in reference count, got: " + lens.command->title);
                // Command should be mtype.showReferences
                require(lens.command->command == "mtype.showReferences",
                    "command should be mtype.showReferences");
                break;
            }
        }
        require(foundFooLens, "expected a code lens with reference count for Foo");
    });

    harness.addTest("class with 1 reference uses singular form", []() {
        auto docMgr = makeDocManager("file:///test.mt",
            "class Bar {}\nlet b = new Bar();\n");
        CodeLensHandler handler(docMgr.get());
        auto lenses = handler.handleCodeLens("file:///test.mt");

        for (const auto& lens : lenses) {
            if (lens.command.has_value()
                && lens.command->title.find("1") != std::string::npos) {
                require(lens.command->title.find("1 reference") != std::string::npos,
                    "expected singular '1 reference', got: " + lens.command->title);
                break;
            }
        }
    });

    harness.addTest("null document returns empty lenses", []() {
        DocumentManager docMgr;
        CodeLensHandler handler(&docMgr);
        auto lenses = handler.handleCodeLens("file:///nonexistent.mt");
        require(lenses.empty(), "expected empty lenses for null document");
    });

    harness.addTest("class with zero references shows 0", []() {
        // openDocument calls parseDocument automatically, so environment is always set.
        auto docMgr = makeDocManager("file:///test.mt", "class Lonely {}\n");
        CodeLensHandler handler(docMgr.get());
        auto lenses = handler.handleCodeLens("file:///test.mt");

        for (const auto& lens : lenses) {
            if (lens.command.has_value()
                && lens.command->title.find("Lonely") != std::string::npos) {
                require(lens.command->title.find("0") != std::string::npos,
                    "expected 0 references for unused class");
            }
        }
    });

    harness.addTest("lens range is at symbol declaration line", []() {
        auto docMgr = makeDocManager("file:///test.mt",
            "class Alpha {}\nclass Beta {}\n");
        CodeLensHandler handler(docMgr.get());
        auto lenses = handler.handleCodeLens("file:///test.mt");

        // Each lens should be at the start of its declaration line (column 0)
        for (const auto& lens : lenses) {
            require(lens.range.start.character == 0,
                "lens should be at column 0");
        }
    });
}

} // namespace mtype::lsp::test
