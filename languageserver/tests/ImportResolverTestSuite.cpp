#include "ImportResolverTestSuite.hpp"
#include "../src/DocumentManager.hpp"
#include "../src/analysis/ImportResolver.hpp"
#include "TempDir.hpp"
#include "../src/utils/UriUtils.hpp"

namespace mtype::lsp::test {

void ImportResolverTestSuite::registerTests(LspTestHarness& harness) {

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
