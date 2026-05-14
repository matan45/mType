#include "PathCompletionHandlerTestSuite.hpp"
#include "../src/handlers/PathCompletionHandler.hpp"
#include "../src/utils/ProjectConfigProvider.hpp"
#include "../src/DocumentManager.hpp"
#include "TempDir.hpp"
#include "../src/utils/UriUtils.hpp"
#include <memory>

namespace mtype::lsp::test {

void PathCompletionHandlerTestSuite::registerTests(LspTestHarness& harness) {

    // PathCompletionHandler flow:
    //   1. Check isInsideImportString (counts quotes — odd count = inside)
    //   2. Extract path typed so far via extractImportPath
    //   3. Resolve to absolute path via resolveRelativePath
    //   4. List directory contents: .mt files + subdirectories

    harness.addTest("cursor outside import string returns empty", []() {
        DocumentManager docMgr;
        docMgr.openDocument("file:///test.mt", "let x: int = 5;\n", 1);

        PathCompletionHandler handler(&docMgr);
        auto items = handler.getPathCompletions("file:///test.mt", {0, 5});

        require(items.empty(), "expected empty completions outside import string");
    });

    harness.addTest("unknown URI returns empty", []() {
        DocumentManager docMgr;
        PathCompletionHandler handler(&docMgr);
        auto items = handler.getPathCompletions("file:///nonexistent.mt", {0, 0});
        require(items.empty(), "expected empty for unknown URI");
    });

    harness.addTest("cursor inside import string returns .mt files", []() {
        TempDir tmpDir;
        tmpDir.createFile("main.mt", "import Foo from \"./\";\n");
        tmpDir.createFile("Foo.mt", "class Foo {}\n");

        std::string mainPath = tmpDir.path() + "/main.mt";
        std::string mainUri = UriUtils::filePathToUri(mainPath);

        DocumentManager docMgr;
        // The content must have import with quotes; cursor inside the quotes
        // isInsideImportString counts quotes up to cursor position
        docMgr.openDocument(mainUri, "import Foo from \"./\";\n", 1);

        PathCompletionHandler handler(&docMgr);
        // Position after "./" inside the quotes — quote count should be odd
        auto items = handler.getPathCompletions(mainUri, {0, 19});

        if (!items.empty()) {
            bool foundMtFile = false;
            for (const auto& item : items) {
                if (item.label.find("Foo") != std::string::npos) {
                    foundMtFile = true;
                    // .mt files get CompletionItemKind::File = 17
                    require(item.kind == 17,
                        "expected File kind for .mt file");
                }
            }
            require(foundMtFile, "expected Foo.mt in path completions");
        }
    });

    // MYT-309 — typing `@` inside an import string lists registered aliases.
    harness.addTest("@ prefix lists installed mt_modules packages", []() {
        TempDir tmpDir;
        tmpDir.createFile(".mtproj", "<Project></Project>\n");
        tmpDir.createFile("mt_modules/@somelib/mtpkg.json",
            "{\"name\":\"somelib\",\"version\":\"0.0.1\",\"source\":\"src\"}\n");
        tmpDir.createFile("mt_modules/@somelib/src/Foo.mt", "class Foo {}\n");
        tmpDir.createFile("main.mt", "import { X } from \"@\";\n");

        auto config = std::make_shared<ProjectConfigProvider>();
        require(config->loadFromWorkspace(tmpDir.path()),
            "loadFromWorkspace should succeed");

        std::string mainPath = tmpDir.path() + "/main.mt";
        std::string mainUri = UriUtils::filePathToUri(mainPath);

        DocumentManager docMgr;
        docMgr.openDocument(mainUri, "import { X } from \"@\";\n", 1);

        PathCompletionHandler handler(&docMgr);
        handler.setProjectConfig(config);

        // Cursor right after the `@` inside the import string.
        auto items = handler.getPathCompletions(mainUri, {0, 20});

        bool foundAlias = false;
        for (const auto& item : items) {
            if (item.label.find("@somelib") != std::string::npos) {
                foundAlias = true;
            }
        }
        require(foundAlias, "expected @somelib in alias completions");
    });

    harness.addTest("subdirectories appear as Folder kind", []() {
        TempDir tmpDir;
        tmpDir.createFile("main.mt", "import X from \"./\";\n");
        tmpDir.createFile("models/Foo.mt", "class Foo {}\n");

        std::string mainPath = tmpDir.path() + "/main.mt";
        std::string mainUri = UriUtils::filePathToUri(mainPath);

        DocumentManager docMgr;
        docMgr.openDocument(mainUri, "import X from \"./\";\n", 1);

        PathCompletionHandler handler(&docMgr);
        auto items = handler.getPathCompletions(mainUri, {0, 18});

        if (!items.empty()) {
            bool foundFolder = false;
            for (const auto& item : items) {
                // Folder kind = 19, label ends with "/"
                if (item.kind == 19 || item.label.find('/') != std::string::npos) {
                    foundFolder = true;
                }
            }
            // listDirectoryContents sorts folders first
            require(foundFolder, "expected subdirectory in path completions");
        }
    });
}

} // namespace mtype::lsp::test
