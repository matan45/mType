#include "CodeActionHandlerTestSuite.hpp"

#include "../src/handlers/CodeActionHandler.hpp"
#include "../src/DocumentManager.hpp"
#include "../src/analysis/WorkspaceSymbolIndex.hpp"
#include "../src/utils/ProjectConfigProvider.hpp"
#include "../src/utils/LSPTypes.hpp"
#include "../src/utils/UriUtils.hpp"
#include "TestFixtures.hpp"
#include "TempDir.hpp"

#include <string>

namespace mtype::lsp::test {

namespace {

std::string editsForImplementAction(const std::vector<CodeAction>& actions) {
    std::string allEdits;
    for (const auto& action : actions) {
        if (action.title.find("Implement interface methods") == std::string::npos) {
            continue;
        }
        if (!action.edit.has_value()) {
            continue;
        }
        for (const auto& [uri, edits] : action.edit->changes) {
            (void)uri;
            for (const auto& edit : edits) {
                allEdits += "---\n" + edit.newText;
            }
        }
    }
    return allEdits;
}

bool hasImplementAction(const std::vector<CodeAction>& actions) {
    for (const auto& action : actions) {
        if (action.title.find("Implement interface methods") != std::string::npos) {
            return true;
        }
    }
    return false;
}

int countOccurrences(const std::string& text, const std::string& needle) {
    int count = 0;
    size_t pos = 0;
    while ((pos = text.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }
    return count;
}

} // namespace

void CodeActionHandlerTestSuite::registerTests(LspTestHarness& harness) {

    // ---------------------------------------------------------------
    // Test 1: Missing import generates a quick-fix CodeAction
    // ---------------------------------------------------------------
    harness.addTest("missing import generates quick-fix with import edit", []() {
        DocumentManager docMgr;
        const std::string mainUri = "file:///test/main.mt";
        const std::string libUri  = "file:///test/lib/Util.mt";

        const std::string mainSource = "let u = new Util();\n";
        docMgr.openDocument(mainUri, mainSource, 1);
        docMgr.parseDocument(mainUri);

        // Seed workspace index with Util in another file
        auto wsIndex = std::make_shared<analysis::WorkspaceSymbolIndex>();
        wsIndex->reindexFile(libUri, "class Util {\n}\n");

        CodeActionHandler handler(&docMgr, wsIndex);

        // Build a diagnostic that mimics what the LSP would produce for
        // an undefined class reference — data.exceptionType = "ClassNotFoundException"
        Diagnostic diag;
        diag.range = {{0, 12}, {0, 16}};  // range covering "Util"
        diag.severity = 1;
        diag.message = "Undefined class 'Util'";
        diag.data = nlohmann::json{
            {"exceptionType", "ClassNotFoundException"}
        };

        auto actions = handler.handleCodeAction(mainUri, diag.range, {diag});

        // Expect at least one quick-fix action for the missing import
        bool foundImportFix = false;
        for (const auto& action : actions) {
            if (action.title.find("Add import") != std::string::npos
                && action.title.find("Util") != std::string::npos) {
                foundImportFix = true;
                require(action.kind.has_value() && *action.kind == "quickfix",
                    "expected kind 'quickfix'");
                require(action.edit.has_value(),
                    "expected CodeAction to have a WorkspaceEdit");
                // The edit should target the main file
                require(action.edit->changes.count(mainUri) == 1,
                    "expected edit changes keyed by main URI");
                require(!action.edit->changes.at(mainUri).empty(),
                    "expected at least one TextEdit");
                const std::string& importEditText =
                    action.edit->changes.at(mainUri)[0].newText;
                require(importEditText.find("import") != std::string::npos,
                    "expected TextEdit newText to contain an import statement");
                // mType uses selective `import { Name } from "...mt";`
                // for auto-generated imports; the test locks in both
                // the brace form and the .mt extension so a regression
                // in either would be caught here.
                require(importEditText.find("import { Util }") != std::string::npos,
                    "expected selective brace-form import for 'Util', got: " + importEditText);
                require(importEditText.find(".mt") != std::string::npos,
                    "expected import path to include .mt extension, got: " + importEditText);
                break;
            }
        }
        require(foundImportFix,
            "expected a 'Add import' CodeAction for missing class 'Util'");
    });

    // ---------------------------------------------------------------
    // Test 2: No self-import — symbol in same file doesn't generate action
    // ---------------------------------------------------------------
    harness.addTest("no self-import for symbol defined in same file", []() {
        DocumentManager docMgr;
        const std::string uri = "file:///test/self.mt";

        const std::string source = "class Foo {}\nlet f = new Foo();\n";
        docMgr.openDocument(uri, source, 1);
        docMgr.parseDocument(uri);

        // Seed workspace index with Foo — but from the same file
        auto wsIndex = std::make_shared<analysis::WorkspaceSymbolIndex>();
        wsIndex->reindexFile(uri, source);

        CodeActionHandler handler(&docMgr, wsIndex);

        Diagnostic diag;
        diag.range = {{1, 12}, {1, 15}};
        diag.severity = 1;
        diag.message = "Undefined class 'Foo'";
        diag.data = nlohmann::json{
            {"exceptionType", "ClassNotFoundException"}
        };

        auto actions = handler.handleCodeAction(uri, diag.range, {diag});

        // No import action should be generated because Foo is in the same file
        for (const auto& action : actions) {
            require(action.title.find("Add import") == std::string::npos
                    || action.title.find("Foo") == std::string::npos,
                "should not generate self-import action for 'Foo' from same file");
        }
    });

    // ---------------------------------------------------------------
    // Test 3: No workspace index — graceful degradation
    // ---------------------------------------------------------------
    harness.addTest("nullptr workspace index: no import fix returned", []() {
        auto docMgr = makeDocManager("file:///test/main.mt", "let x = new Unknown();\n");
        CodeActionHandler handler(docMgr.get()); // nullptr workspace index

        auto diag = makeDiagnostic({{0, 12}, {0, 19}},
            "Undefined class 'Unknown'", "ClassNotFoundException");
        auto actions = handler.handleCodeAction("file:///test/main.mt", diag.range, {diag});

        for (const auto& action : actions) {
            require(action.title.find("Add import") == std::string::npos,
                "should not generate import fix without workspace index");
        }
    });

    // ---------------------------------------------------------------
    // Test 4: Typo fix from suggestions
    // ---------------------------------------------------------------
    harness.addTest("typo fix: diagnostic with suggestions produces fix action", []() {
        auto docMgr = makeDocManager("file:///test/typo.mt",
            "class Greeter {}\nlet g = new Greete();\n");
        CodeActionHandler handler(docMgr.get());

        auto diag = makeDiagnosticWithSuggestions(
            {{1, 12}, {1, 18}}, "Undefined class 'Greete'", {"Greeter"});

        auto actions = handler.handleCodeAction("file:///test/typo.mt", diag.range, {diag});

        bool foundTypoFix = false;
        for (const auto& action : actions) {
            if (action.edit.has_value()) {
                for (const auto& [uri, edits] : action.edit->changes) {
                    for (const auto& edit : edits) {
                        if (edit.newText.find("Greeter") != std::string::npos) {
                            foundTypoFix = true;
                        }
                    }
                }
            }
        }
        require(foundTypoFix,
            "expected a typo fix action replacing 'Greete' with 'Greeter'");
    });

    // ---------------------------------------------------------------
    // Test 5: Multiple candidates from workspace index
    // ---------------------------------------------------------------
    harness.addTest("multiple workspace matches produce multiple import actions", []() {
        auto docMgr = makeDocManager("file:///test/main.mt", "let h = new Helper();\n");

        auto wsIndex = std::make_shared<analysis::WorkspaceSymbolIndex>();
        wsIndex->reindexFile("file:///a/Helper.mt", "class Helper {}\n");
        wsIndex->reindexFile("file:///b/Helper.mt", "class Helper {}\n");

        CodeActionHandler handler(docMgr.get(), wsIndex);
        auto diag = makeDiagnostic({{0, 12}, {0, 18}},
            "Undefined class 'Helper'", "ClassNotFoundException");

        auto actions = handler.handleCodeAction("file:///test/main.mt", diag.range, {diag});

        int importActionCount = 0;
        for (const auto& action : actions) {
            if (action.title.find("Add import") != std::string::npos
                && action.title.find("Helper") != std::string::npos) {
                ++importActionCount;
            }
        }
        require(importActionCount >= 2,
            "expected at least 2 import actions for 2 workspace matches, got "
            + std::to_string(importActionCount));
    });

    // ---------------------------------------------------------------
    // Test 6: Implement interface action
    // ---------------------------------------------------------------
    harness.addTest("implement interface: generates stub methods", []() {
        auto docMgr = makeDocManager("file:///test/impl.mt",
            "interface Printable {\n    function toString(): string;\n}\n"
            "class Widget implements Printable {\n}\n");
        CodeActionHandler handler(docMgr.get());

        // Request code actions at the class declaration line
        Range range{{3, 0}, {3, 40}};
        auto actions = handler.handleCodeAction("file:///test/impl.mt", range, {});

        bool foundImplement = false;
        for (const auto& action : actions) {
            if (action.title.find("Implement") != std::string::npos
                || action.title.find("implement") != std::string::npos) {
                foundImplement = true;
                if (action.edit.has_value()) {
                    for (const auto& [uri, edits] : action.edit->changes) {
                        for (const auto& edit : edits) {
                            require(edit.newText.find("toString") != std::string::npos,
                                "stub should contain 'toString' method");
                            // Lock in the modifier + signature shape so a
                            // regression to the old `function name(...)` form
                            // (no `public`) surfaces immediately.
                            require(edit.newText.find("public function toString(): string")
                                    != std::string::npos,
                                "stub should be `public function toString(): string`, got: "
                                + edit.newText);
                            require(edit.newText.find("@Override") != std::string::npos,
                                "stub should be annotated `@Override`");
                        }
                    }
                }
                break;
            }
        }
        require(foundImplement,
            "expected an 'Implement interface' code action");
    });

    // ---------------------------------------------------------------
    // Test 6b: Implement interface — params use C/Java syntax and
    // referenced external types are auto-imported alongside the stub.
    // ---------------------------------------------------------------
    harness.addTest("implement interface: C/Java param syntax + auto-imports", []() {
        DocumentManager docMgr;
        const std::string animalUri = "file:///test/Animal.mt";
        const std::string mainUri   = "file:///test/Keeper.mt";

        // Keep the interface inline with the implementing class so the
        // test doesn't depend on cross-file import resolution (the
        // default DocumentManager in tests has no importResolver wired
        // up, so `import { ... } from "..."` would not actually pull
        // the interface into the document's environment). The Animal
        // type is referenced in the signature but never imported —
        // that's exactly the case the auto-import path should fix.
        const std::string animalSrc = "class Animal {\n}\n";
        const std::string mainSrc =
            "interface Caretaker {\n"
            "    function feed(Animal a, int count): bool;\n"
            "}\n"
            "class Keeper implements Caretaker {\n}\n";

        docMgr.openDocument(animalUri, animalSrc, 1);
        docMgr.parseDocument(animalUri);
        docMgr.openDocument(mainUri, mainSrc, 1);
        docMgr.parseDocument(mainUri);

        auto wsIndex = std::make_shared<analysis::WorkspaceSymbolIndex>();
        wsIndex->reindexFile(animalUri, animalSrc);
        wsIndex->reindexFile(mainUri, mainSrc);

        // Sanity-check the index — if Animal isn't indexed, no later
        // assertion can succeed and we want a clear failure message
        // pointing at the index, not at the auto-import logic.
        auto animalMatches = wsIndex->findByName("Animal", 5);
        require(!animalMatches.empty(),
            "WorkspaceSymbolIndex should contain 'Animal' after reindexFile");

        CodeActionHandler handler(&docMgr, wsIndex);
        // The implementing class declaration is on line 3 (0-based)
        // — interface header (0), method (1), interface close (2),
        // class declaration (3).
        Range range{{3, 0}, {3, 40}};
        auto actions = handler.handleCodeAction(mainUri, range, {});

        bool foundImplement = false;
        bool sawCJavaParams = false;
        bool sawAnimalImport = false;
        // Capture all generated edits so a failed assertion can show
        // exactly what the handler produced — much faster to diagnose
        // than guessing at which filter rejected the type.
        std::string allEditsConcat;
        for (const auto& action : actions) {
            if (action.title.find("Implement") == std::string::npos
                && action.title.find("implement") == std::string::npos) continue;
            foundImplement = true;
            if (!action.edit.has_value()) continue;
            for (const auto& [uri, edits] : action.edit->changes) {
                for (const auto& edit : edits) {
                    allEditsConcat += "---\n" + edit.newText;
                    if (edit.newText.find("public function feed(Animal a, int count): bool")
                        != std::string::npos) {
                        sawCJavaParams = true;
                    }
                    if (edit.newText.find("import { Animal }") != std::string::npos
                        && edit.newText.find(".mt") != std::string::npos) {
                        sawAnimalImport = true;
                    }
                }
            }
        }
        require(foundImplement, "expected an Implement-interface action");
        require(sawCJavaParams,
            "stub should use C/Java param syntax `Animal a, int count`. "
            "All edits seen:\n" + allEditsConcat);
        require(sawAnimalImport,
            "auto-import should attach `import { Animal } from \"...mt\"` "
            "for the externally-defined parameter type. All edits seen:\n"
            + allEditsConcat);
    });

    harness.addTest("implement interface: multiple interfaces and partial implementation", []() {
        auto docMgr = makeDocManager("file:///test/multi.mt",
            "interface Drawable {\n"
            "    function draw(): void;\n"
            "}\n"
            "interface Resizable {\n"
            "    function resize(int width, int height): void;\n"
            "}\n"
            "class Widget implements Drawable, Resizable {\n"
            "    public function draw(): void {\n"
            "    }\n"
            "}\n");
        CodeActionHandler handler(docMgr.get());

        Range range{{6, 0}, {6, 52}};
        auto actions = handler.handleCodeAction("file:///test/multi.mt", range, {});
        const std::string edits = editsForImplementAction(actions);

        require(hasImplementAction(actions),
            "expected an Implement-interface action for missing Resizable.resize");
        require(edits.find("public function resize(int width, int height): void")
                != std::string::npos,
            "should generate missing resize method. Edits:\n" + edits);
        require(edits.find("public function draw(): void") == std::string::npos,
            "should not regenerate already implemented draw method. Edits:\n" + edits);
    });

    harness.addTest("implement interface: inherited interface methods", []() {
        auto docMgr = makeDocManager("file:///test/inherited.mt",
            "interface Base {\n"
            "    function baseMethod(): int;\n"
            "}\n"
            "interface Child extends Base {\n"
            "    function childMethod(): string;\n"
            "}\n"
            "class Impl implements Child {\n"
            "}\n");
        CodeActionHandler handler(docMgr.get());

        Range range{{6, 0}, {6, 35}};
        auto actions = handler.handleCodeAction("file:///test/inherited.mt", range, {});
        const std::string edits = editsForImplementAction(actions);

        require(edits.find("public function baseMethod(): int") != std::string::npos,
            "should include inherited Base.baseMethod. Edits:\n" + edits);
        require(edits.find("public function childMethod(): string") != std::string::npos,
            "should include direct Child.childMethod. Edits:\n" + edits);
        require(countOccurrences(edits, "@Override") == 2,
            "should annotate each generated method with @Override. Edits:\n" + edits);
    });

    harness.addTest("implement interface: overloaded methods only generate missing overload", []() {
        auto docMgr = makeDocManager("file:///test/overload.mt",
            "interface Formatter {\n"
            "    function format(int value): string;\n"
            "    function format(string value): string;\n"
            "}\n"
            "class TextFormatter implements Formatter {\n"
            "    public function format(int value): string {\n"
            "        return \"\";\n"
            "    }\n"
            "}\n");
        CodeActionHandler handler(docMgr.get());

        Range range{{4, 0}, {4, 48}};
        auto actions = handler.handleCodeAction("file:///test/overload.mt", range, {});
        const std::string edits = editsForImplementAction(actions);

        require(edits.find("public function format(string value): string")
                != std::string::npos,
            "should generate missing string overload. Edits:\n" + edits);
        require(edits.find("public function format(int value): string")
                == std::string::npos,
            "should not regenerate existing int overload. Edits:\n" + edits);
    });

    harness.addTest("implement interface: generic interface substitutes type arguments", []() {
        // MYT-360 — primitives are no longer accepted as generic type
        // arguments, so this fixture uses the boxed class `String` instead
        // of the primitive `string`. The substitution mechanic is identical;
        // only the type token changes.
        auto docMgr = makeDocManager("file:///test/generic.mt",
            "interface Box<T> {\n"
            "    function get(): T;\n"
            "    function set(T value): void;\n"
            "}\n"
            "class StringBox implements Box<String> {\n"
            "}\n");
        CodeActionHandler handler(docMgr.get());

        Range range{{4, 0}, {4, 42}};
        auto actions = handler.handleCodeAction("file:///test/generic.mt", range, {});
        const std::string edits = editsForImplementAction(actions);

        require(edits.find("public function get(): String") != std::string::npos,
            "generic return type T should be substituted with String. Edits:\n" + edits);
        require(edits.find("public function set(String value): void") != std::string::npos,
            "generic parameter type T should be substituted with String. Edits:\n" + edits);
    });

    harness.addTest("implement interface: diagnostic inside class still offers action", []() {
        auto docMgr = makeDocManager("file:///test/diagnostic.mt",
            "interface Runnable {\n"
            "    function run(): void;\n"
            "}\n"
            "class Job implements Runnable {\n"
            "    int id = 1;\n"
            "}\n");
        CodeActionHandler handler(docMgr.get());

        Diagnostic diag;
        diag.range = {{4, 4}, {4, 10}};
        diag.severity = 1;
        diag.message = "Class 'Job' must implement interface method 'run'";
        diag.data = nlohmann::json{{"exceptionType", "MissingInterfaceMethod"}};

        auto actions = handler.handleCodeAction("file:///test/diagnostic.mt", diag.range, {diag});
        const std::string edits = editsForImplementAction(actions);

        require(hasImplementAction(actions),
            "expected diagnostic-triggered Implement-interface action");
        require(edits.find("public function run(): void") != std::string::npos,
            "diagnostic-triggered action should generate run stub. Edits:\n" + edits);
    });

    harness.addTest("implement interface: no action when all methods implemented", []() {
        auto docMgr = makeDocManager("file:///test/complete.mt",
            "interface Runnable {\n"
            "    function run(): void;\n"
            "}\n"
            "class Job implements Runnable {\n"
            "    public function run(): void {\n"
            "    }\n"
            "}\n");
        CodeActionHandler handler(docMgr.get());

        Range range{{3, 0}, {3, 32}};
        auto actions = handler.handleCodeAction("file:///test/complete.mt", range, {});
        require(!hasImplementAction(actions),
            "should not offer implement action when every method is already present");
    });

    harness.addTest("implement interface: mt_modules package type auto-import uses alias", []() {
        TempDir tmpDir;
        tmpDir.createFile(".mtproj", "<Project></Project>\n");
        tmpDir.createFile("src/Main.mt", "");
        tmpDir.createFile("mt_modules/@animals/mtpkg.json",
            "{\"name\":\"animals\",\"version\":\"0.0.1\",\"source\":\"src\"}\n");
        tmpDir.createFile("mt_modules/@animals/src/Animal.mt",
            "class Animal {\n"
            "}\n");
        tmpDir.createFile("mt_modules/@animals/src/Caretaker.mt",
            "interface Caretaker {\n"
            "    function feed(Animal animal): void;\n"
            "}\n");

        const std::string mainPath = (fs::path(tmpDir.path()) / "src" / "Main.mt").string();
        const std::string animalPath =
            (fs::path(tmpDir.path()) / "mt_modules" / "@animals" / "src" / "Animal.mt").string();
        const std::string caretakerPath =
            (fs::path(tmpDir.path()) / "mt_modules" / "@animals" / "src" / "Caretaker.mt").string();
        const std::string mainUri = UriUtils::filePathToUri(mainPath);
        const std::string animalUri = UriUtils::filePathToUri(animalPath);
        const std::string caretakerUri = UriUtils::filePathToUri(caretakerPath);

        auto config = std::make_shared<ProjectConfigProvider>();
        require(config->loadFromWorkspace(tmpDir.path()),
            "test project config should load .mtproj and mt_modules aliases");

        DocumentManager docMgr;
        docMgr.setProjectConfig(config);
        const std::string mainSrc =
            "import { Caretaker } from \"@animals/Caretaker.mt\";\n"
            "class Keeper implements Caretaker {\n"
            "}\n";
        docMgr.openDocument(mainUri, mainSrc, 1);
        docMgr.parseDocument(mainUri);

        auto wsIndex = std::make_shared<analysis::WorkspaceSymbolIndex>();
        wsIndex->reindexFile(animalUri,
            "class Animal {\n"
            "}\n");
        wsIndex->reindexFile(caretakerUri,
            "interface Caretaker {\n"
            "    function feed(Animal animal): void;\n"
            "}\n");

        CodeActionHandler handler(&docMgr, wsIndex);
        handler.setProjectConfig(config);

        Range range{{1, 0}, {1, 40}};
        auto actions = handler.handleCodeAction(mainUri, range, {});
        const std::string edits = editsForImplementAction(actions);

        require(edits.find("public function feed(Animal animal): void")
                != std::string::npos,
            "should generate package interface method. Edits:\n" + edits);
        require(edits.find("import { Animal } from \"@animals/Animal.mt\";")
                != std::string::npos,
            "should auto-import package type through @animals alias. Edits:\n" + edits);
    });

    // ---------------------------------------------------------------
    // Test 7: No implement action when no implements clause
    // ---------------------------------------------------------------
    harness.addTest("no implement action for class without implements", []() {
        auto docMgr = makeDocManager("file:///test/plain.mt", "class Plain {}\n");
        CodeActionHandler handler(docMgr.get());

        Range range{{0, 0}, {0, 14}};
        auto actions = handler.handleCodeAction("file:///test/plain.mt", range, {});

        for (const auto& action : actions) {
            require(action.title.find("Implement") == std::string::npos,
                "should not offer implement action for class without 'implements'");
        }
    });

    // ---------------------------------------------------------------
    // MYT-364: structured-edits path honours producer-supplied range
    // and never expands into the next line's identifier.
    //
    // Repro: the "insert ';'" suggestion produced by
    // ExceptionConverter::convertMissingSemicolon anchors the diagnostic
    // at the next token (e.g. the start of `Int` on line 1). Before the
    // fix, generateTypoFixActions ignored the suggestion's explicit
    // zero-width edit, fed the diagnostic anchor through wordRangeAt,
    // and emitted a TextEdit that REPLACED the `Int` identifier with
    // `;`. This test pins the new behaviour: the structured edit must
    // round-trip verbatim — same range, same newText, no expansion.
    // ---------------------------------------------------------------
    harness.addTest("MYT-364: structured ';' insert does not overwrite next identifier", []() {
        const std::string uri = "file:///test/missing_semi.mt";
        auto docMgr = makeDocManager(uri,
            "foo()\n"
            "Int x = 1;\n");
        CodeActionHandler handler(docMgr.get());

        // Diagnostic anchored at the start of `Int` on line 1 (the next
        // token after the missing `;`), plus a structured zero-width
        // insert at the same position carrying `";"` as the newText —
        // exactly what ExceptionConverter emits for
        // MissingSemicolonException.
        TextEdit insertSemi;
        insertSemi.range = {{1, 0}, {1, 0}};
        insertSemi.newText = ";";

        auto diag = makeDiagnosticWithStructuredEdits(
            {{1, 0}, {1, 1}},
            "expected ';' at end of statement",
            "insert ';'",
            {insertSemi});

        auto actions = handler.handleCodeAction(uri, diag.range, {diag});

        bool foundInsert = false;
        for (const auto& action : actions) {
            if (action.title != "Insert ';'") continue;
            foundInsert = true;
            require(action.kind.has_value() && *action.kind == "quickfix",
                "expected kind 'quickfix' for Insert ';'");
            require(action.edit.has_value(),
                "expected Insert ';' action to carry a WorkspaceEdit");
            const auto& edits = action.edit->changes.at(uri);
            require(edits.size() == 1,
                "expected exactly one TextEdit for Insert ';', got "
                + std::to_string(edits.size()));
            const auto& te = edits.front();
            // The critical pins. A regression that re-introduces the
            // wordRangeAt expansion would fail every one of these.
            require(te.newText == ";",
                "newText should be ';' verbatim, got '" + te.newText + "'");
            require(te.range.start.line == 1 && te.range.start.character == 0,
                "edit start must be (1,0), got ("
                + std::to_string(te.range.start.line) + ","
                + std::to_string(te.range.start.character) + ")");
            require(te.range.end.line == 1 && te.range.end.character == 0,
                "edit end must equal start for a zero-width insert, got ("
                + std::to_string(te.range.end.line) + ","
                + std::to_string(te.range.end.character) + ")");
            require(te.range.start.line == te.range.end.line,
                "edit must NOT span a newline (MYT-364 regression)");
            require(!(te.range.start.line == 1
                      && te.range.end.character >= 3),
                "edit must NOT cover the `Int` identifier on line 1 "
                "(MYT-364 regression)");
        }
        require(foundInsert,
            "expected an 'Insert \\';\\'' code action from the structured "
            "edit suggestion");
    });

    // ---------------------------------------------------------------
    // MYT-364: back-compat. Legacy label-only suggestions (no `edits`
    // field, e.g. did-you-mean typo fixes) must still route through
    // the wordRangeAt expansion path and produce a "Replace with '<x>'"
    // action covering the full identifier under the diagnostic anchor.
    // ---------------------------------------------------------------
    harness.addTest("MYT-364 back-compat: legacy typo path still expands to full word", []() {
        const std::string uri = "file:///test/typo_legacy.mt";
        auto docMgr = makeDocManager(uri,
            "class Greeter {}\n"
            "let g = new Greete();\n");
        CodeActionHandler handler(docMgr.get());

        // Range pins the diagnostic anchor inside the misspelled word
        // `Greete` on line 1 (chars 12..18). The legacy synthesis path
        // should expand the edit to cover the full identifier.
        auto diag = makeDiagnosticWithSuggestions(
            {{1, 12}, {1, 18}},
            "Undefined class 'Greete'",
            {"Greeter"});

        auto actions = handler.handleCodeAction(uri, diag.range, {diag});

        bool foundReplace = false;
        for (const auto& action : actions) {
            if (action.title != "Replace with 'Greeter'") continue;
            foundReplace = true;
            require(action.edit.has_value(),
                "expected legacy typo action to carry a WorkspaceEdit");
            const auto& edits = action.edit->changes.at(uri);
            require(edits.size() == 1,
                "expected exactly one TextEdit for legacy typo path");
            const auto& te = edits.front();
            require(te.newText == "Greeter",
                "newText should be the candidate identifier");
            // The range should cover the full `Greete` identifier (6
            // chars), not just the 1-character diagnostic anchor.
            require(te.range.start.line == 1 && te.range.start.character == 12,
                "edit start should be (1,12) — start of `Greete`");
            require(te.range.end.line == 1 && te.range.end.character == 18,
                "edit end should be (1,18) — end of `Greete`");
        }
        require(foundReplace,
            "expected a 'Replace with \\'Greeter\\'' action via legacy path");
    });

    // ---------------------------------------------------------------
    // MYT-364: a present-but-empty `edits` array signals a producer
    // mistake; the handler must NOT degrade to a no-op action AND must
    // NOT fall through to the label-based wordRangeAt synthesis — that
    // path is exactly the one that overwrites the next identifier when
    // the label is "insert ';'". Both `Insert ';'` (would-be structured)
    // and `Replace with ';'` (would-be legacy) must be absent.
    // ---------------------------------------------------------------
    harness.addTest("MYT-364: empty edits array does not produce a no-op action", []() {
        const std::string uri = "file:///test/empty_edits.mt";
        auto docMgr = makeDocManager(uri, "Int x = 1;\n");
        CodeActionHandler handler(docMgr.get());

        auto diag = makeDiagnosticWithStructuredEdits(
            {{0, 0}, {0, 1}},
            "expected ';' at end of statement",
            "insert ';'",
            {} /* empty edits */);

        auto actions = handler.handleCodeAction(uri, diag.range, {diag});

        for (const auto& action : actions) {
            require(action.title != "Insert ';'",
                "should not produce 'Insert \\';\\'' action when edits "
                "array is empty");
            require(action.title != "Replace with ';'",
                "must not fall through to legacy wordRangeAt synthesis "
                "for an empty structured-edits suggestion — that path "
                "re-introduces the MYT-364 overwrite");
        }
    });

    // ---------------------------------------------------------------
    // MYT-360: PrimitiveInGenericException quick-fix
    // ---------------------------------------------------------------
    harness.addTest("primitive-in-generic: rewrites int to Int and adds import", []() {
        const std::string mainUri = "file:///test/main.mt";
        const std::string intUri  = "file:///test/lib/primitives/Int.mt";

        const std::string mainSource =
            "interface Predicate<T> {\n"
            "    function test(T value): bool;\n"
            "}\n"
            "class PositivePredicate implements Predicate<int> {\n"
            "}\n";

        auto docMgr = makeDocManager(mainUri, mainSource);

        auto wsIndex = std::make_shared<analysis::WorkspaceSymbolIndex>();
        wsIndex->reindexFile(intUri, "public class Int {\n}\n");

        CodeActionHandler handler(docMgr.get(), wsIndex);

        // Diagnostic anchored on the line `class PositivePredicate implements
        // Predicate<int>` near the `int` token. The handler scans the line
        // for the primitive whole-word, so the exact column tolerance is
        // forgiving by design.
        Diagnostic diag = makeDiagnostic(
            {{3, 53}, {3, 56}},
            "Primitive type 'int' cannot be used as generic type argument. "
            "Use boxed type 'Int' instead",
            "PrimitiveInGenericException");

        auto actions = handler.handleCodeAction(mainUri, diag.range, {diag});

        bool foundRewrite = false;
        bool foundImport = false;
        for (const auto& action : actions) {
            if (action.title.find("Replace 'int' with 'Int'") == std::string::npos) {
                continue;
            }
            require(action.kind.has_value() && *action.kind == "quickfix",
                "expected kind 'quickfix'");
            require(action.edit.has_value(),
                "expected primitive-in-generic action to carry a WorkspaceEdit");
            const auto& edits = action.edit->changes.at(mainUri);
            for (const auto& edit : edits) {
                if (edit.newText == "Int"
                    && edit.range.start.line == 3
                    && edit.range.end.character - edit.range.start.character == 3)
                {
                    foundRewrite = true;
                }
                if (edit.newText.find("import") != std::string::npos
                    && edit.newText.find("Int") != std::string::npos)
                {
                    foundImport = true;
                }
            }
        }
        require(foundRewrite,
            "expected a TextEdit replacing 'int' with 'Int' on the implements line");
        require(foundImport,
            "expected a TextEdit inserting an Int import");
    });
}

} // namespace mtype::lsp::test
