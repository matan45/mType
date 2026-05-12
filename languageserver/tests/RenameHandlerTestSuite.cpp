#include "RenameHandlerTestSuite.hpp"

#include <algorithm>
#include <memory>
#include <string>

#include "../src/DocumentManager.hpp"
#include "../src/analysis/WorkspaceSymbolIndex.hpp"
#include "../src/handlers/RenameHandler.hpp"
#include "TestFixtures.hpp"

namespace mtype::lsp::test {

namespace {

// Helper: open two documents in one manager and feed them to the
// workspace index so cross-file rename tests can find the second file.
struct TwoFileFixture {
    std::unique_ptr<DocumentManager> docMgr;
    std::shared_ptr<analysis::WorkspaceSymbolIndex> index;
};

TwoFileFixture makeTwoFileFixture(const std::string& uriA, const std::string& srcA,
                                    const std::string& uriB, const std::string& srcB) {
    TwoFileFixture f;
    f.docMgr = std::make_unique<DocumentManager>();
    f.docMgr->openDocument(uriA, srcA, 1);
    f.docMgr->openDocument(uriB, srcB, 1);
    f.docMgr->parseDocument(uriA);
    f.docMgr->parseDocument(uriB);
    f.index = std::make_shared<analysis::WorkspaceSymbolIndex>();
    f.index->reindexFile(uriA, srcA);
    f.index->reindexFile(uriB, srcB);
    return f;
}

// Count text edits across every URI in a workspace edit.
int totalEdits(const WorkspaceEdit& we) {
    int n = 0;
    for (const auto& [_, edits] : we.changes) n += static_cast<int>(edits.size());
    return n;
}

// Find the edits for one URI (returns empty if none).
const std::vector<TextEdit>* editsFor(const WorkspaceEdit& we, const std::string& uri) {
    auto it = we.changes.find(uri);
    if (it == we.changes.end()) return nullptr;
    return &it->second;
}

} // namespace

void RenameHandlerTestSuite::registerTests(LspTestHarness& harness) {

    // ---- Valid-rename cases ----

    harness.addTest("renames a local variable end-to-end", []() {
        const std::string src =
            "int count = 5;\n"
            "print(count);\n";
        auto docMgr = makeDocManager("file:///test.mt", src);
        RenameHandler handler(docMgr.get());

        auto prep = handler.prepareRename("file:///test.mt", {0, 4});
        require(prep.ok, "prepareRename should accept the local: " + prep.error);

        auto res = handler.rename("file:///test.mt", {0, 4}, "total");
        require(res.ok, "rename should succeed: " + res.error);
        require(totalEdits(res.edit) == 2,
            "expected 2 edits (decl + usage), got " + std::to_string(totalEdits(res.edit)));
        const auto* edits = editsFor(res.edit, "file:///test.mt");
        require(edits != nullptr, "expected edits for the test URI");
        for (const auto& e : *edits) {
            require(e.newText == "total", "edit text should be 'total'");
        }
    });

    harness.addTest("renames a top-level class in one file", []() {
        const std::string src =
            "class Foo {\n"
            "    public constructor() {}\n"
            "}\n"
            "Foo a;\n"
            "Foo b = new Foo();\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        auto res = handler.rename("file:///t.mt", {0, 6}, "Bar");
        require(res.ok, "rename should succeed: " + res.error);
        // 4 occurrences of Foo: declaration, type-annotation (line 3),
        // type-annotation (line 4), `new Foo` (line 4).
        require(totalEdits(res.edit) == 4,
            "expected 4 edits, got " + std::to_string(totalEdits(res.edit)));
    });

    harness.addTest("renames a class across two files", []() {
        const std::string srcA =
            "class Foo {\n"
            "    public constructor() {}\n"
            "}\n";
        const std::string srcB =
            "Foo x;\n"
            "Foo y = new Foo();\n";
        auto f = makeTwoFileFixture("file:///a.mt", srcA, "file:///b.mt", srcB);
        RenameHandler handler(f.docMgr.get(), f.index);

        auto res = handler.rename("file:///a.mt", {0, 6}, "Bar");
        require(res.ok, "rename should succeed: " + res.error);
        require(editsFor(res.edit, "file:///a.mt") != nullptr,
            "expected edits in file A");
        require(editsFor(res.edit, "file:///b.mt") != nullptr,
            "expected edits in file B");
        // a.mt: declaration. b.mt: 3 usages (two type annotations + new).
        require(totalEdits(res.edit) >= 4,
            "expected >= 4 cross-file edits, got " + std::to_string(totalEdits(res.edit)));
    });

    harness.addTest("renames a top-level function", []() {
        const std::string src =
            "function greet(): int { return 1; }\n"
            "greet();\n"
            "greet();\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        auto res = handler.rename("file:///t.mt", {0, 9}, "salute");
        require(res.ok, "rename should succeed: " + res.error);
        require(totalEdits(res.edit) == 3,
            "expected 3 edits, got " + std::to_string(totalEdits(res.edit)));
    });

    harness.addTest("renames a method whose override is in a subclass (same file)", []() {
        // Both classes in the same file so ClassRegistry sees the
        // inheritance link without import resolution.
        const std::string src =
            "class Parent {\n"
            "    public function foo(): int { return 1; }\n"
            "}\n"
            "class Child extends Parent {\n"
            "    public function foo(): int { return 2; }\n"
            "}\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        // Cursor on `foo` in Parent (line 1, col 21 — inside "foo" which
        // starts at col 20 of "    public function foo(...)").
        auto res = handler.rename("file:///t.mt", {1, 21}, "bar");
        require(res.ok, "rename should succeed: " + res.error);
        // Both `foo` declarations should be renamed.
        require(totalEdits(res.edit) == 2,
            "expected 2 edits across override pair, got " + std::to_string(totalEdits(res.edit)));
    });

    // ---- Rejected-target cases ----

    harness.addTest("rejects rename when cursor is on a keyword", []() {
        const std::string src = "class Foo {}\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        auto prep = handler.prepareRename("file:///t.mt", {0, 1});
        require(!prep.ok, "prepareRename should reject keyword");
    });

    harness.addTest("rejects rename when cursor is on whitespace", []() {
        const std::string src = "int x = 1;\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        auto prep = handler.prepareRename("file:///t.mt", {0, 3});  // space
        require(!prep.ok, "prepareRename should reject whitespace cursor");
    });

    harness.addTest("rejects rename when cursor is on a numeric literal", []() {
        const std::string src = "int x = 42;\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        auto prep = handler.prepareRename("file:///t.mt", {0, 9});  // on '4'
        require(!prep.ok, "prepareRename should reject numeric literal");
    });

    harness.addTest("rejects rename when cursor is on a string literal", []() {
        const std::string src = "string s = \"hello\";\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        auto prep = handler.prepareRename("file:///t.mt", {0, 13});  // inside "hello"
        require(!prep.ok, "prepareRename should reject string literal");
    });

    harness.addTest("rejects rename when new name is invalid", []() {
        const std::string src = "int x = 1;\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        auto res = handler.rename("file:///t.mt", {0, 4}, "123foo");
        require(!res.ok, "rename should reject invalid identifier");
    });

    harness.addTest("rejects rename when new name is reserved", []() {
        const std::string src = "int x = 1;\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        auto res = handler.rename("file:///t.mt", {0, 4}, "class");
        require(!res.ok, "rename should reject reserved keyword as new name");
    });

    harness.addTest("rejects rename when new name is empty", []() {
        const std::string src = "int x = 1;\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        auto res = handler.rename("file:///t.mt", {0, 4}, "");
        require(!res.ok, "rename should reject empty new name");
    });

    harness.addTest("rejects rename when symbol is shared with a member of an unrelated class", []() {
        // Two unrelated classes both declare a method `foo`. Renaming
        // one would silently rewrite identifier tokens that belong to
        // the other — the v1 ambiguity gate must catch this.
        const std::string src =
            "class A {\n"
            "    public function foo(): int { return 1; }\n"
            "}\n"
            "class B {\n"
            "    public function foo(): int { return 2; }\n"
            "}\n";
        auto f = makeTwoFileFixture("file:///t.mt", src, "file:///dummy.mt", "");
        RenameHandler handler(f.docMgr.get(), f.index);

        auto res = handler.rename("file:///t.mt", {1, 21}, "bar");
        require(!res.ok, "rename should reject ambiguous member name");
    });

    // ---- No-false-positive (canary) cases ----

    harness.addTest("does not modify a comment containing the old name", []() {
        // Comments are dropped by the lexer entirely, so they never
        // produce IDENTIFIER tokens and cannot appear in edits — this
        // test guards against the day someone changes that.
        const std::string src =
            "// count is the running total\n"
            "int count = 1;\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        auto res = handler.rename("file:///t.mt", {1, 4}, "total");
        require(res.ok, "rename should succeed: " + res.error);
        const auto* edits = editsFor(res.edit, "file:///t.mt");
        require(edits != nullptr, "expected edits for the test URI");
        for (const auto& e : *edits) {
            require(e.range.start.line != 0,
                "edit must not touch the comment on line 0");
        }
    });

    harness.addTest("does not modify a string literal containing the old name", []() {
        const std::string src =
            "string s = \"count me in\";\n"
            "int count = 1;\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        auto res = handler.rename("file:///t.mt", {1, 4}, "total");
        require(res.ok, "rename should succeed: " + res.error);
        const auto* edits = editsFor(res.edit, "file:///t.mt");
        require(edits != nullptr, "expected edits for the test URI");
        for (const auto& e : *edits) {
            require(e.range.start.line != 0,
                "edit must not touch the string literal on line 0");
        }
    });

    harness.addTest("substring boundary: renames 'count' without touching 'counter'", []() {
        const std::string src =
            "int count = 1;\n"
            "int counter = 2;\n"
            "count = 5;\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        // Cursor on `count` (line 0). `counter` is a separately-declared
        // local, which would trip the local-ambiguity guard — but the
        // guard counts identifier-after-type-token decls of the SAME
        // name only, so the two distinct names should both pass.
        auto res = handler.rename("file:///t.mt", {0, 4}, "total");
        require(res.ok, "rename should succeed: " + res.error);
        const auto* edits = editsFor(res.edit, "file:///t.mt");
        require(edits != nullptr, "expected edits for the test URI");
        // Verify no edit overlaps `counter` (which starts at line 1
        // col 4 and has length 7).
        for (const auto& e : *edits) {
            bool overlapsCounter =
                e.range.start.line == 1 &&
                e.range.start.character == 4;
            require(!overlapsCounter,
                "edit must not touch 'counter' on line 1");
        }
    });

    harness.addTest("rejects rename when a local is declared twice in one file", []() {
        const std::string src =
            "function f(): void { int x = 1; }\n"
            "function g(): void { int x = 2; }\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        // Cursor on the first `x` (col 25 of the line "function f(): void
        // { int x = 1; }"). The handler should refuse because `x` has two
        // declaration sites and v1 doesn't do scope resolution.
        auto res = handler.rename("file:///t.mt", {0, 25}, "y");
        require(!res.ok, "rename should reject ambiguous local declared in two scopes");
    });

    harness.addTest("rejects rename when the same top-level name exists in another file", []() {
        const std::string srcA =
            "class Foo {\n"
            "    public constructor() {}\n"
            "}\n";
        const std::string srcB =
            "class Foo {\n"
            "    public constructor() {}\n"
            "}\n";
        auto f = makeTwoFileFixture("file:///a.mt", srcA, "file:///b.mt", srcB);
        RenameHandler handler(f.docMgr.get(), f.index);

        auto res = handler.rename("file:///a.mt", {0, 6}, "Bar");
        require(!res.ok, "rename should reject duplicate top-level name across files");
    });

    harness.addTest("renamed range matches identifier length exactly", []() {
        const std::string src = "int abc = 10;\nprint(abc);\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        auto res = handler.rename("file:///t.mt", {0, 4}, "xyz");
        require(res.ok, "rename should succeed: " + res.error);
        const auto* edits = editsFor(res.edit, "file:///t.mt");
        require(edits != nullptr, "expected edits for the test URI");
        for (const auto& e : *edits) {
            int len = e.range.end.character - e.range.start.character;
            require(len == 3,
                "range length must equal old identifier length, got " + std::to_string(len));
        }
    });

    harness.addTest("no-op when newName equals oldName", []() {
        const std::string src = "int x = 1;\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        RenameHandler handler(docMgr.get());

        auto res = handler.rename("file:///t.mt", {0, 4}, "x");
        require(res.ok, "no-op rename should succeed");
        require(totalEdits(res.edit) == 0, "no-op rename should produce zero edits");
    });

    harness.addTest("returns empty for unknown URI", []() {
        auto docMgr = makeDocManager("file:///t.mt", "int x = 1;\n");
        RenameHandler handler(docMgr.get());

        auto prep = handler.prepareRename("file:///unknown.mt", {0, 0});
        require(!prep.ok, "prepareRename should reject unknown URI");
    });
}

} // namespace mtype::lsp::test
