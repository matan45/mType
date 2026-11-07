#include "WorkspaceSymbolIndexTestSuite.hpp"
#include "../src/analysis/WorkspaceSymbolIndex.hpp"
#include "../src/utils/UriUtils.hpp"
#include "TempDir.hpp"

#include <atomic>
#include <cctype>
#include <filesystem>
#include <thread>
#include <vector>

namespace mtype::lsp::test {

namespace {

std::string uriOf(const TempDir& dir, const std::string& relativePath) {
    return UriUtils::filePathToUri(
        (std::filesystem::path(dir.path()) / relativePath).string());
}

void registerListAndRevertTests(LspTestHarness& harness) {

    harness.addTest("findByPrefix with an empty prefix stays empty", []() {
        // AutoImportCompletionProvider relies on this; the empty-query
        // listing for workspace/symbol goes through listAll instead.
        analysis::WorkspaceSymbolIndex index;
        index.reindexFile("file:///a.mt", "class Alpha {}\n");
        require(index.findByPrefix("", 20).empty(),
            "empty prefix must not enumerate the index");
    });

    harness.addTest("listAll returns every symbol, sorted and capped", []() {
        analysis::WorkspaceSymbolIndex index;
        index.reindexFile("file:///b.mt", "function beta(): void {}\nclass Gamma {}\n");
        index.reindexFile("file:///a.mt", "class Alpha {}\n");

        auto all = index.listAll(10);
        require(all.size() == 3, "expected 3, got " + std::to_string(all.size()));
        require(all[0].name == "Alpha" && all[1].name == "beta" && all[2].name == "Gamma",
            "expected case-insensitive alphabetical order");

        auto capped = index.listAll(2);
        require(capped.size() == 2 && capped[1].name == "beta",
            "expected the first two in ranked order");
        require(index.listAll(0).empty(), "maxResults 0 returns nothing");
    });

    harness.addTest("revertToDisk replaces unsaved-buffer symbols with the disk version", []() {
        // MYT-447 — closing a tab used to drop the file's symbols entirely.
        TempDir root;
        root.createFile("Dog.mt", "class Dog {}\n");
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());
        const std::string uri = uriOf(root, "Dog.mt");

        index.reindexFile(uri, "class Puppy {}\n"); // the edited, unsaved buffer
        require(index.findByName("Dog").empty(), "buffer reindex should replace Dog");

        index.revertToDisk(uri);
        require(index.findByName("Dog").size() == 1, "Dog should be back after close");
        require(index.findByName("Puppy").empty(), "unsaved Puppy should be gone");
    });

    harness.addTest("revertToDisk drops a file that was deleted", []() {
        TempDir root;
        root.createFile("Gone.mt", "class Gone {}\n");
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());
        require(index.findByName("Gone").size() == 1, "precondition: Gone indexed");

        std::filesystem::remove(std::filesystem::path(root.path()) / "Gone.mt");
        index.revertToDisk(uriOf(root, "Gone.mt"));
        require(index.findByName("Gone").empty(), "deleted file must not linger");
    });

    harness.addTest("revertToDisk drops a file outside the workspace", []() {
        TempDir root;
        root.createFile("Inside.mt", "class Inside {}\n");
        TempDir elsewhere;
        elsewhere.createFile("Outside.mt", "class Outside {}\n");
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());

        const std::string outsideUri = uriOf(elsewhere, "Outside.mt");
        index.reindexFile(outsideUri, "class Outside {}\n"); // opened ad hoc
        require(index.findByName("Outside").size() == 1, "precondition: Outside indexed");

        index.revertToDisk(outsideUri);
        require(index.findByName("Outside").empty(),
            "a closed out-of-workspace file must leave Ctrl+T");
        require(index.findByName("Inside").size() == 1, "workspace symbols untouched");
    });
}

// MYT-432 — freshness rules: one key per file whatever the client spelling,
// a failed parse keeps the last good entries, and an open buffer wins over
// disk until it is closed.
void registerFreshnessTests(LspTestHarness& harness) {

    harness.addTest("a c%3A URI reindexes and drops one entry", []() {
        analysis::WorkspaceSymbolIndex index;
        const std::string vscodeUri = "file:///c%3A/ws/Foo.mt";
        index.reindexFile(vscodeUri, "class Foo {}\n");
        index.reindexFile(vscodeUri, "class Foo {}\n");
        require(index.findByName("Foo").size() == 1,
            "expected 1 Foo, got " + std::to_string(index.findByName("Foo").size()));

        index.invalidateFile(vscodeUri);
        require(index.findByName("Foo").empty(), "invalidateFile must drop the c%3A entry");
    });

    harness.addTest("a parse error keeps the last good entries", []() {
        analysis::WorkspaceSymbolIndex index;
        index.reindexFile("file:///a.mt", "class Foo {}\nclass Extra {}\n");
        index.reindexFile("file:///a.mt", "class Foo implements Bar {\n"); // unterminated
        require(index.findByName("Foo").size() == 1, "Foo must survive a broken buffer");
        require(index.findByName("Extra").size() == 1, "the whole last good parse survives");

        index.reindexFile("file:///a.mt", "class Foo {}\n");
        require(index.findByName("Extra").empty(), "the next good parse replaces it");
    });

    harness.addTest("an open buffer wins over a disk reindex", []() {
        TempDir root;
        root.createFile("Dog.mt", "class Dog {}\n");
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());
        const std::string uri = uriOf(root, "Dog.mt");

        index.openFile(uri, "class Puppy {}\n");
        index.reindexFile(uri); // file-watcher event: disk still says Dog
        require(index.findByName("Puppy").size() == 1, "the buffer's Puppy must stay");
        require(index.findByName("Dog").empty(), "disk must not overwrite the open buffer");
    });

    harness.addTest("an open buffer outlives its file on disk until closed", []() {
        TempDir root;
        root.createFile("Dog.mt", "class Dog {}\n");
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());
        const std::string uri = uriOf(root, "Dog.mt");
        index.openFile(uri, "class Puppy {}\n");

        std::filesystem::remove(std::filesystem::path(root.path()) / "Dog.mt");
        index.invalidateFile(uri); // delete event while the tab is open
        require(index.findByName("Puppy").size() == 1, "the open buffer keeps its symbols");

        index.revertToDisk(uri); // the tab closes
        require(index.findByName("Puppy").empty(), "closing a deleted file drops it");
        require(index.findByName("Dog").empty(), "nothing comes back from disk");
    });

    harness.addTest("the workspace scan skips a file that is already open", []() {
        TempDir root;
        root.createFile("Dog.mt", "class Dog {}\n");
        analysis::WorkspaceSymbolIndex index;
        index.openFile(uriOf(root, "Dog.mt"), "class Puppy {}\n"); // a restored dirty tab
        index.buildFromWorkspace(root.path());
        require(index.findByName("Puppy").size() == 1, "the buffer must survive the scan");
        require(index.findByName("Dog").empty(), "the scan must not load the disk version");
    });

    harness.addTest("after close, disk events update the file again", []() {
        TempDir root;
        root.createFile("Dog.mt", "class Dog {}\n");
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());
        const std::string uri = uriOf(root, "Dog.mt");
        index.openFile(uri, "class Puppy {}\n");
        index.revertToDisk(uri);
        require(index.findByName("Dog").size() == 1, "precondition: Dog is back after close");

        root.createFile("Dog.mt", "class Wolf {}\n");
        index.reindexFile(uri);
        require(index.findByName("Wolf").size() == 1, "disk wins once the buffer is closed");
        require(index.findByName("Dog").empty(), "the old disk version is replaced");
    });

    harness.addTest("closing onto a broken disk file keeps the last good entries", []() {
        TempDir root;
        root.createFile("Dog.mt", "class Dog {\n"); // broken on disk
        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(root.path());
        require(index.findByName("Dog").empty(), "precondition: the broken file indexes nothing");

        const std::string uri = uriOf(root, "Dog.mt");
        index.openFile(uri, "class Dog {}\n");
        index.revertToDisk(uri);
        require(index.findByName("Dog").size() == 1, "the buffer's Dog is the last good entry");
    });

    harness.addTest("a VS Code URI and a lower-case drive root share one entry", []() {
        TempDir root;
        root.createFile("Dog.mt", "class Dog {}\n");
        std::string rootPath = root.path();
        if (rootPath.size() < 2 || rootPath[1] != ':') return; // drive letters are Windows-only

        // VS Code sends a lower-case root and file:///c%3A/... document URIs.
        rootPath[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(rootPath[0])));
        const std::string upperUri = uriOf(root, "Dog.mt"); // file:///C:/...
        const std::string vscodeUri = "file:///" + std::string(1, rootPath[0]) + "%3A" + upperUri.substr(10);

        analysis::WorkspaceSymbolIndex index;
        index.buildFromWorkspace(rootPath);
        index.openFile(vscodeUri, "class Dog {}\nclass Puppy {}\n");
        index.revertToDisk(vscodeUri);
        require(index.findByName("Dog").size() == 1,
            "expected exactly 1 Dog, got " + std::to_string(index.findByName("Dog").size()));
        require(index.findByName("Puppy").empty(), "the closed buffer's Puppy must be gone");
    });
}

} // namespace

void WorkspaceSymbolIndexTestSuite::registerTests(LspTestHarness& harness) {

    registerListAndRevertTests(harness);
    registerFreshnessTests(harness);

    harness.addTest("default constructor: findByName returns empty", []() {
        analysis::WorkspaceSymbolIndex index;
        auto results = index.findByName("Anything");
        require(results.empty(), "expected empty for unindexed name");
    });

    harness.addTest("reindexFile makes class findable by name", []() {
        analysis::WorkspaceSymbolIndex index;
        index.reindexFile("file:///lib/Foo.mt", "class Foo {}\n");
        auto results = index.findByName("Foo");
        require(results.size() == 1, "expected 1 result for 'Foo'");
        require(results[0].name == "Foo", "name mismatch");
        require(results[0].fileUri == "file:///lib/Foo.mt", "fileUri mismatch");
    });

    harness.addTest("reindexFile indexes multiple classes", []() {
        analysis::WorkspaceSymbolIndex index;
        index.reindexFile("file:///multi.mt",
            "class Alpha {}\nclass Beta {}\nfunction gamma(): void {}\n");
        require(!index.findByName("Alpha").empty(), "Alpha should be indexed");
        require(!index.findByName("Beta").empty(), "Beta should be indexed");
        require(!index.findByName("gamma").empty(), "gamma should be indexed");
    });

    harness.addTest("invalidateFile removes entries", []() {
        analysis::WorkspaceSymbolIndex index;
        index.reindexFile("file:///lib/Foo.mt", "class Foo {}\n");
        require(!index.findByName("Foo").empty(), "Foo should exist before invalidate");

        index.invalidateFile("file:///lib/Foo.mt");
        require(index.findByName("Foo").empty(), "Foo should be gone after invalidate");
    });

    harness.addTest("reindexFile replaces old entries (no duplicates)", []() {
        analysis::WorkspaceSymbolIndex index;
        index.reindexFile("file:///lib/Foo.mt", "class Foo {}\n");
        index.reindexFile("file:///lib/Foo.mt", "class Foo {}\nclass Bar {}\n");

        auto fooResults = index.findByName("Foo");
        require(fooResults.size() == 1, "expected exactly 1 Foo after re-index, got "
            + std::to_string(fooResults.size()));
        require(!index.findByName("Bar").empty(), "Bar should exist after re-index");
    });

    harness.addTest("findByName respects maxResults cap", []() {
        analysis::WorkspaceSymbolIndex index;
        // Same name in different files
        index.reindexFile("file:///a.mt", "class Dup {}\n");
        index.reindexFile("file:///b.mt", "class Dup {}\n");
        index.reindexFile("file:///c.mt", "class Dup {}\n");

        auto results = index.findByName("Dup", 2);
        require(results.size() == 2, "expected max 2 results, got "
            + std::to_string(results.size()));
    });

    harness.addTest("computeImportSpelling: sibling file", []() {
        auto spelling = analysis::WorkspaceSymbolIndex::computeImportSpelling(
            "C:/project/Foo.mt", "C:/project/Main.mt");
        require(spelling.find("Foo") != std::string::npos,
            "spelling should contain 'Foo'");
        // mType requires the explicit .mt suffix in import paths
        // (the parser rejects extensionless imports — see
        // tests/testFiles/import/error/importMissingExtension.mt).
        require(spelling.find(".mt") != std::string::npos,
            "spelling should contain .mt extension");
        // Sibling imports use a leading ./ so the parser treats them
        // as relative rather than search-path lookups.
        require(spelling.rfind("./", 0) == 0,
            "sibling spelling should start with './'");
    });

    harness.addTest("thread safety: concurrent reindex and find do not crash", []() {
        analysis::WorkspaceSymbolIndex index;

        // Seed initial data
        index.reindexFile("file:///initial.mt", "class Initial {}\n");

        std::atomic<bool> go{false};
        std::atomic<bool> crashed{false};
        std::vector<std::thread> threads;

        // Multiple readers
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&]() {
                while (!go.load()) {} // spin until all threads are ready
                try {
                    for (int j = 0; j < 100; ++j) {
                        auto r = index.findByName("Initial");
                        (void)r;
                    }
                } catch (...) {
                    crashed.store(true);
                }
            });
        }

        // One writer
        threads.emplace_back([&]() {
            while (!go.load()) {} // spin until all threads are ready
            try {
                for (int j = 0; j < 50; ++j) {
                    index.reindexFile("file:///writer.mt",
                        "class Writer" + std::to_string(j) + " {}\n");
                }
            } catch (...) {
                crashed.store(true);
            }
        });

        go.store(true); // release all threads simultaneously

        for (auto& t : threads) t.join();

        require(!crashed.load(), "concurrent access should not crash");
    });
}

} // namespace mtype::lsp::test
