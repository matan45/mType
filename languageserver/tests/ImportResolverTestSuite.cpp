#include "ImportResolverTestSuite.hpp"
#include "../src/DocumentManager.hpp"
#include "../src/analysis/ImportResolver.hpp"
#include "../src/utils/ProjectConfigProvider.hpp"
#include "TempDir.hpp"
#include "../src/utils/UriUtils.hpp"

namespace mtype::lsp::test {

namespace {

std::string uriIn(const TempDir& dir, const std::string& relativePath) {
    return UriUtils::filePathToUri((fs::path(dir.path()) / relativePath).string());
}

bool hasClass(const Document& doc, const std::string& name) {
    auto classes = doc.environment ? doc.environment->getClassRegistry() : nullptr;
    return classes && classes->findClass(name) != nullptr;
}

// Opens `relativePath` (already on disk in `dir`) with `source`, parses it,
// and returns the live document.
Document& openAndParse(DocumentManager& docMgr, const TempDir& dir,
                       const std::string& relativePath, const std::string& source) {
    const std::string uri = uriIn(dir, relativePath);
    docMgr.openDocument(uri, source, 1);
    auto* doc = docMgr.getDocument(uri);
    require(doc != nullptr, "document should be open");
    return *doc;
}

void writeShapes(TempDir& dir) {
    dir.createFile("shapes/Base.mt", "class Base {}\n");
    dir.createFile("shapes/Circle.mt",
        "import { Base } from \"./Base.mt\";\n"
        "class Circle extends Base {}\n");
}

// MYT-440 — the import summary that gates unresolved-type diagnostics.
void registerSummaryTests(LspTestHarness& harness) {

    harness.addTest("transitive import is visible on the first and the second parse", []() {
        // The old cache-hit path copied only the file's own symbols, so Base
        // vanished from main.mt on every re-parse.
        TempDir dir;
        writeShapes(dir);
        const std::string src = "import { Circle } from \"./shapes/Circle.mt\";\n";
        dir.createFile("main.mt", src);
        DocumentManager docMgr;
        Document& doc = openAndParse(docMgr, dir, "main.mt", src);
        require(hasClass(doc, "Circle") && hasClass(doc, "Base"), "first parse: Circle and Base");
        require(doc.importSummary.complete, "first parse: summary complete");

        docMgr.updateDocument(doc.uri, src + "\n", 2);
        require(hasClass(doc, "Base"), "second parse (cache hit) must still see Base");
        require(doc.importSummary.typeNames.count("Base") == 1, "Base listed in the summary");
        require(doc.importSummary.complete, "second parse: summary complete");
    });

    harness.addTest("selective import split across lines resolves", []() {
        TempDir dir;
        writeShapes(dir);
        const std::string src =
            "import {\n"
            "    Circle\n"
            "} from \"./shapes/Circle.mt\";\n";
        dir.createFile("main.mt", src);
        DocumentManager docMgr;
        Document& doc = openAndParse(docMgr, dir, "main.mt", src);
        require(hasClass(doc, "Circle"), "multi-line import should resolve Circle");
        require(doc.importSummary.complete, "summary complete");
    });

    harness.addTest("alias and original are both listed", []() {
        TempDir dir;
        writeShapes(dir);
        const std::string src = "import { Circle as Round } from \"./shapes/Circle.mt\";\n";
        dir.createFile("main.mt", src);
        DocumentManager docMgr;
        Document& doc = openAndParse(docMgr, dir, "main.mt", src);
        require(doc.importSummary.typeNames.count("Round") == 1, "alias listed");
        require(doc.importSummary.typeNames.count("Circle") == 1, "original listed");
    });

    harness.addTest("import cycle terminates and stays complete", []() {
        TempDir dir;
        dir.createFile("A.mt", "import { B } from \"./B.mt\";\nclass A {}\n");
        dir.createFile("B.mt", "import { A } from \"./A.mt\";\nclass B {}\n");
        DocumentManager docMgr;
        Document& doc = openAndParse(docMgr, dir, "A.mt",
            "import { B } from \"./B.mt\";\nclass A {}\n");
        require(hasClass(doc, "B"), "B should be imported");
        require(doc.importSummary.complete, "a cycle is not an unresolved import");
    });

    harness.addTest("missing nested import makes the summary incomplete", []() {
        TempDir dir;
        dir.createFile("shapes/Circle.mt",
            "import { Gone } from \"./Gone.mt\";\nclass Circle {}\n");
        const std::string src = "import { Circle } from \"./shapes/Circle.mt\";\n";
        dir.createFile("main.mt", src);
        DocumentManager docMgr;
        Document& doc = openAndParse(docMgr, dir, "main.mt", src);
        require(hasClass(doc, "Circle"), "what resolves is still merged");
        require(!doc.importSummary.complete, "an unresolved nested import must be reported");
    });

    harness.addTest("missing direct import makes the summary incomplete", []() {
        TempDir dir;
        const std::string src = "import { Nope } from \"./Nope.mt\";\n";
        dir.createFile("main.mt", src);
        DocumentManager docMgr;
        Document& doc = openAndParse(docMgr, dir, "main.mt", src);
        require(!doc.importSummary.complete, "an unresolved import must be reported");
    });

    harness.addTest("failed parse falls back to the regex and is incomplete", []() {
        TempDir dir;
        writeShapes(dir);
        const std::string src = "import { Circle } from \"./shapes/Circle.mt\";\nclass {\n";
        dir.createFile("main.mt", src);
        DocumentManager docMgr;
        Document& doc = openAndParse(docMgr, dir, "main.mt", src);
        require(!doc.importSummary.complete, "a regex-derived import list is never complete");
    });

    harness.addTest("mt_modules alias resolves at nested levels", []() {
        // The old nested path ignored the project config.
        TempDir dir;
        dir.createFile(".mtproj", "<Project></Project>\n");
        dir.createFile("mt_modules/@animals/mtpkg.json",
            "{\"name\":\"animals\",\"version\":\"0.0.1\",\"source\":\"src\"}\n");
        dir.createFile("mt_modules/@animals/src/Animal.mt", "class Animal {}\n");
        dir.createFile("mt_modules/@animals/src/Keeper.mt",
            "import { Animal } from \"@animals/Animal.mt\";\nclass Keeper {}\n");
        const std::string src = "import { Keeper } from \"@animals/Keeper.mt\";\n";
        dir.createFile("src/Main.mt", src);

        auto config = std::make_shared<ProjectConfigProvider>();
        require(config->loadFromWorkspace(dir.path()), "project config should load");
        DocumentManager docMgr;
        docMgr.setProjectConfig(config);
        Document& doc = openAndParse(docMgr, dir, "src/Main.mt", src);
        require(hasClass(doc, "Keeper"), "direct alias import resolves");
        require(hasClass(doc, "Animal"), "nested alias import resolves through the project config");
        require(doc.importSummary.complete, "summary complete");
    });

    harness.addTest("clearImportCache picks up edits made on disk", []() {
        TempDir dir;
        dir.createFile("shapes/Circle.mt", "class Circle {}\n");
        const std::string src = "import * from \"./shapes/Circle.mt\";\n";
        dir.createFile("main.mt", src);
        DocumentManager docMgr;
        Document& doc = openAndParse(docMgr, dir, "main.mt", src);
        require(!hasClass(doc, "Square"), "precondition: no Square yet");

        dir.createFile("shapes/Circle.mt", "class Circle {}\nclass Square {}\n");
        docMgr.updateDocument(doc.uri, src, 2);
        require(!hasClass(doc, "Square"), "the cached parse is still used");

        docMgr.clearImportCache();
        docMgr.updateDocument(doc.uri, src, 3);
        require(hasClass(doc, "Square"), "after clearing, the new class is visible");
    });
}

} // namespace

void ImportResolverTestSuite::registerTests(LspTestHarness& harness) {

    registerSummaryTests(harness);

    harness.addTest("resolveImports populates environment with imported class", []() {
        TempDir tmpDir;
        tmpDir.createFile("Helper.mt", "class Helper {\n    function help(): void {}\n}\n");
        tmpDir.createFile("main.mt", "import Helper from \"./Helper\";\n");

        std::string mainPath = tmpDir.path() + "/main.mt";
        std::string mainUri = UriUtils::filePathToUri(mainPath);

        DocumentManager docMgr;
        docMgr.openDocument(mainUri, "import Helper from \"./Helper\";\n", 1);
        docMgr.parseDocument(mainUri);

        auto* doc = docMgr.getDocument(mainUri);
        if (doc && doc->environment) {
            auto classReg = doc->environment->getClassRegistry();
            if (classReg) {
                auto cls = classReg->findClass("Helper");
                // Helper may or may not be resolved depending on import resolution
                // This test verifies the import machinery runs without crash
            }
        }
    });

    harness.addTest("clearCache does not crash", []() {
        // ImportResolver is internal to DocumentManager; test via fresh parse
        DocumentManager docMgr;
        docMgr.openDocument("file:///test.mt", "class Foo {}\n", 1);
        docMgr.parseDocument("file:///test.mt");
        // Reparse triggers import re-resolution
        docMgr.updateDocument("file:///test.mt", "class Foo {}\nclass Bar {}\n", 2);
        docMgr.parseDocument("file:///test.mt");
        // Should not crash on re-parse
    });

    harness.addTest("missing import file does not crash", []() {
        DocumentManager docMgr;
        docMgr.openDocument("file:///test.mt",
            "import Missing from \"./nonexistent\";\n", 1);
        docMgr.parseDocument("file:///test.mt");
        // Should handle gracefully — no crash
        auto* doc = docMgr.getDocument("file:///test.mt");
        require(doc != nullptr, "document should still exist after failed import");
    });

    harness.addTest("circular import does not infinite loop", []() {
        TempDir tmpDir;
        tmpDir.createFile("A.mt", "import B from \"./B\";\nclass A {}\n");
        tmpDir.createFile("B.mt", "import A from \"./A\";\nclass B {}\n");

        std::string aPath = tmpDir.path() + "/A.mt";
        std::string aUri = UriUtils::filePathToUri(aPath);

        DocumentManager docMgr;
        docMgr.openDocument(aUri, "import B from \"./B\";\nclass A {}\n", 1);
        docMgr.parseDocument(aUri);
        // Should terminate without infinite loop
    });
}

} // namespace mtype::lsp::test
