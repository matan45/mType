#include "CodeLensHandlerTestSuite.hpp"
#include "../src/handlers/CodeLensHandler.hpp"
#include "../src/DocumentManager.hpp"
#include "TestFixtures.hpp"
#include "../../mType/environment/registry/ClassDefinition.hpp"

#include <string>

namespace mtype::lsp::test {

void CodeLensHandlerTestSuite::registerTests(LspTestHarness& harness) {

    // CodeLens uses local AST class/interface declarations, counts references
    // in code text only, and creates lenses with format:
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

    harness.addTest("class with zero references produces no lens", []() {
        // openDocument calls parseDocument automatically, so environment is always set.
        auto docMgr = makeDocManager("file:///test.mt", "class Lonely {}\n");
        CodeLensHandler handler(docMgr.get());
        auto lenses = handler.handleCodeLens("file:///test.mt");

        require(lenses.empty(), "unused local class should not produce a 0 references lens");
    });

    harness.addTest("lens range is at symbol declaration line", []() {
        auto docMgr = makeDocManager("file:///test.mt",
            "class Alpha {}\n"
            "class Beta {}\n"
            "Alpha a = new Alpha();\n"
            "Beta b = new Beta();\n");
        CodeLensHandler handler(docMgr.get());
        auto lenses = handler.handleCodeLens("file:///test.mt");

        // Each lens should be at the start of its declaration line (column 0)
        for (const auto& lens : lenses) {
            require(lens.range.start.character == 0,
                "lens should be at column 0");
        }
    });

    harness.addTest("imported class symbols do not create lenses in current document", []() {
        auto docMgr = makeDocManager("file:///test.mt",
            "class Local {}\n"
            "Local local = new Local();\n"
            "// Imported declaration line should not render here\n"
            "function main(): void {\n"
            "    Imported value = new Imported();\n"
            "}\n");
        auto* doc = docMgr->getDocument("file:///test.mt");
        require(doc != nullptr, "expected test document");
        require(doc->environment != nullptr, "expected parsed environment");

        auto classRegistry = doc->environment->getClassRegistry();
        require(classRegistry != nullptr, "expected class registry");
        classRegistry->registerClass(
            "Imported",
            std::make_shared<runtimeTypes::klass::ClassDefinition>("Imported"));
        doc->symbolLocations["Imported"] = SymbolLocationInfo{
            "file:///lib/Imported.mt",
            1,
            0,
            ""
        };

        CodeLensHandler handler(docMgr.get());
        auto lenses = handler.handleCodeLens("file:///test.mt");

        for (const auto& lens : lenses) {
            require(lens.range.start.line != 2,
                "imported class lens should not render on current document comments");
        }
    });

    harness.addTest("static calls inside a loop do not create usage-site lenses", []() {
        const std::string src =
            "class Local {}\n"
            "function tick(): void {\n"
            "    while (acc >= fixedDt) {\n"
            "        Combat::run(reg, world, fixedDt);\n"
            "        Gather::run(reg, world, fixedDt);\n"
            "        Power::refresh(reg);\n"
            "    }\n"
            "}\n";
        auto docMgr = makeDocManager("file:///test.mt", src);

        auto* doc = docMgr->getDocument("file:///test.mt");
        require(doc != nullptr, "expected test document");
        require(doc->environment != nullptr, "expected parsed environment");
        auto classRegistry = doc->environment->getClassRegistry();
        require(classRegistry != nullptr, "expected class registry");
        for (const auto& name : {"Combat", "Gather", "Power"}) {
            classRegistry->registerClass(
                name,
                std::make_shared<runtimeTypes::klass::ClassDefinition>(name));
            doc->symbolLocations[name] = SymbolLocationInfo{
                "file:///lib/" + std::string(name) + ".mt",
                3,
                0,
                ""
            };
        }

        CodeLensHandler handler(docMgr.get());
        auto lenses = handler.handleCodeLens("file:///test.mt");

        require(lenses.empty(),
            "static call usages and unused local declarations should not produce lenses");
    });

    harness.addTest("comment-only references do not create a lens", []() {
        auto docMgr = makeDocManager("file:///test.mt",
            "class Ghost {}\n"
            "// Ghost appears only in a comment\n"
            "function main(): void {}\n");
        CodeLensHandler handler(docMgr.get());
        auto lenses = handler.handleCodeLens("file:///test.mt");

        require(lenses.empty(), "comment-only references should not count");
    });
}

} // namespace mtype::lsp::test
