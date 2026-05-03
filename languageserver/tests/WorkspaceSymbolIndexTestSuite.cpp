#include "WorkspaceSymbolIndexTestSuite.hpp"
#include "../src/analysis/WorkspaceSymbolIndex.hpp"

#include <atomic>
#include <thread>
#include <vector>

namespace mtype::lsp::test {

void WorkspaceSymbolIndexTestSuite::registerTests(LspTestHarness& harness) {

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
