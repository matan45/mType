#include "WorkspaceSymbolHandlerTestSuite.hpp"

#include <algorithm>
#include <chrono>
#include <future>
#include <memory>
#include <string>

#include "../src/analysis/WorkspaceSymbolIndex.hpp"
#include "../src/handlers/WorkspaceSymbolHandler.hpp"
#include "../src/utils/LSPTypes.hpp"

namespace mtype::lsp::test {

namespace {

std::shared_ptr<analysis::WorkspaceSymbolIndex> makeIndex() {
    return std::make_shared<analysis::WorkspaceSymbolIndex>();
}

const SymbolInformation* findByName(const std::vector<SymbolInformation>& syms,
                                    const std::string& name) {
    for (const auto& s : syms) {
        if (s.name == name) return &s;
    }
    return nullptr;
}

int countByKind(const std::vector<SymbolInformation>& syms, SymbolKind kind) {
    int n = 0;
    for (const auto& s : syms) {
        if (s.kind == kind) ++n;
    }
    return n;
}

} // namespace

void WorkspaceSymbolHandlerTestSuite::registerTests(LspTestHarness& harness) {

    harness.addTest("exact match returns the symbol by name", []() {
        auto index = makeIndex();
        index->reindexFile("file:///lib/Foo.mt", "class Foo {}\n");
        index->reindexFile("file:///lib/Bar.mt", "class Bar {}\n");
        WorkspaceSymbolHandler handler(index);

        auto syms = handler.handleWorkspaceSymbol("Foo");
        require(syms.size() == 1,
            "expected 1 result for 'Foo', got " + std::to_string(syms.size()));
        require(syms[0].name == "Foo", "expected name 'Foo'");
        require(syms[0].location.uri == "file:///lib/Foo.mt",
            "expected uri to be Foo.mt");
        require(syms[0].kind == SymbolKind::Class, "expected Class kind");
    });

    harness.addTest("prefix match returns all symbols starting with prefix", []() {
        auto index = makeIndex();
        index->reindexFile("file:///a.mt",
            "class Box {}\nclass Boat {}\nclass Cup {}\n");
        WorkspaceSymbolHandler handler(index);

        auto syms = handler.handleWorkspaceSymbol("Bo");
        require(syms.size() == 2,
            "expected 2 prefix matches, got " + std::to_string(syms.size()));
        require(findByName(syms, "Box") != nullptr, "expected 'Box' in results");
        require(findByName(syms, "Boat") != nullptr, "expected 'Boat' in results");
        require(findByName(syms, "Cup") == nullptr, "did not expect 'Cup'");
    });

    harness.addTest("matching is case-insensitive", []() {
        auto index = makeIndex();
        index->reindexFile("file:///a.mt", "class Bar {}\n");
        WorkspaceSymbolHandler handler(index);

        auto syms = handler.handleWorkspaceSymbol("bAr");
        require(syms.size() == 1,
            "expected 1 case-insensitive match, got " + std::to_string(syms.size()));
        require(syms[0].name == "Bar",
            "expected original spelling 'Bar' returned, got '" + syms[0].name + "'");
    });

    harness.addTest("empty query returns no results", []() {
        // Mirrors WorkspaceSymbolIndex::findByPrefix, which short-circuits
        // on empty input rather than streaming every symbol. VS Code's
        // Ctrl+T picker handles an empty response by waiting for the user
        // to type — same UX as completion.
        auto index = makeIndex();
        index->reindexFile("file:///a.mt",
            "class Alpha {}\nclass Beta {}\nfunction gamma(): void {}\n");
        WorkspaceSymbolHandler handler(index);

        auto syms = handler.handleWorkspaceSymbol("");
        require(syms.empty(),
            "expected empty result for empty query, got "
            + std::to_string(syms.size()));
    });

    harness.addTest("single-character query still matches", []() {
        // Boundary case: the spec doesn't require a minimum query length,
        // and the existing prefix matcher returns results from one char up.
        auto index = makeIndex();
        index->reindexFile("file:///a.mt", "class Alpha {}\nclass Apple {}\n");
        WorkspaceSymbolHandler handler(index);

        auto syms = handler.handleWorkspaceSymbol("A");
        require(syms.size() == 2,
            "expected 2 matches for single-char query, got "
            + std::to_string(syms.size()));
    });

    harness.addTest("results span multiple files", []() {
        auto index = makeIndex();
        index->reindexFile("file:///lib/a.mt", "class Shared {}\n");
        index->reindexFile("file:///lib/b.mt", "class Shared {}\n");
        WorkspaceSymbolHandler handler(index);

        auto syms = handler.handleWorkspaceSymbol("Shared");
        require(syms.size() == 2,
            "expected 2 cross-file matches, got " + std::to_string(syms.size()));

        bool sawA = false;
        bool sawB = false;
        for (const auto& s : syms) {
            if (s.location.uri == "file:///lib/a.mt") sawA = true;
            if (s.location.uri == "file:///lib/b.mt") sawB = true;
        }
        require(sawA && sawB, "expected results from both files");
    });

    harness.addTest("unknown query returns empty list", []() {
        auto index = makeIndex();
        index->reindexFile("file:///a.mt", "class Foo {}\n");
        WorkspaceSymbolHandler handler(index);

        auto syms = handler.handleWorkspaceSymbol("Nonexistent");
        require(syms.empty(), "expected no results for unmatched query");
    });

    harness.addTest("kind mapping covers class, interface, and function", []() {
        auto index = makeIndex();
        index->reindexFile("file:///a.mt",
            "class K {}\ninterface I {}\nfunction f(): void {}\n");
        WorkspaceSymbolHandler handler(index);

        auto klass = handler.handleWorkspaceSymbol("K");
        require(klass.size() == 1 && klass[0].kind == SymbolKind::Class,
            "expected 'K' to be Class");

        auto iface = handler.handleWorkspaceSymbol("I");
        require(iface.size() == 1 && iface[0].kind == SymbolKind::Interface,
            "expected 'I' to be Interface");

        auto fn = handler.handleWorkspaceSymbol("f");
        require(fn.size() == 1 && fn[0].kind == SymbolKind::Function,
            "expected 'f' to be Function");
    });

    harness.addTest("unready index does not hang the handler", []() {
        auto index = makeIndex();
        // Seed at least one symbol so we can verify the partial-result
        // path returns it under a pending future without waiting it out.
        index->reindexFile("file:///a.mt", "class Pending {}\n");

        // Wire a future that never completes within the wait window.
        // waitForReady's 50 ms cap is internal to the handler — assert
        // we return well inside a reasonable budget regardless of the
        // future state.
        std::promise<void> never;
        index->setReadyFuture(never.get_future().share());

        WorkspaceSymbolHandler handler(index);

        const auto start = std::chrono::steady_clock::now();
        auto syms = handler.handleWorkspaceSymbol("Pending");
        const auto elapsed = std::chrono::steady_clock::now() - start;

        require(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
                < 500,
            "handler must return promptly even when the index is unready");
        require(syms.size() == 1,
            "expected the already-indexed symbol to come back as a partial result");
    });

    harness.addTest("range anchors on the symbol name", []() {
        auto index = makeIndex();
        index->reindexFile("file:///a.mt", "class Anchored {}\n");
        WorkspaceSymbolHandler handler(index);

        auto syms = handler.handleWorkspaceSymbol("Anchored");
        require(syms.size() == 1, "expected exactly 1 result");

        const auto& range = syms[0].location.range;
        require(range.start.line == range.end.line,
            "expected single-line range");
        require(range.end.character - range.start.character
                == static_cast<int>(std::string("Anchored").size()),
            "expected range to span exactly the symbol name");
    });
}

} // namespace mtype::lsp::test
