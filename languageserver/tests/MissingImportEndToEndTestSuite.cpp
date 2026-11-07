#include "MissingImportEndToEndTestSuite.hpp"
#include "../src/DocumentManager.hpp"
#include "../src/analysis/WorkspaceSymbolIndex.hpp"
#include "../src/handlers/CodeActionHandler.hpp"
#include "../src/handlers/CodeActionMerger.hpp"
#include "../src/handlers/DiagnosticsHandler.hpp"
#include "../src/utils/LSPTypes.hpp"
#include "../src/utils/UriUtils.hpp"
#include "TempDir.hpp"

#include <memory>
#include <string>
#include <vector>

namespace mtype::lsp::test {

namespace {

const std::string kCircleImport = "import { Circle } from \"./shapes/Circle.mt\";\n";

// A workspace with shapes/Circle.mt, an index that knows it, and main.mt
// opened with `mainSource`.
struct Workspace {
    TempDir dir;
    DocumentManager docMgr;
    std::shared_ptr<analysis::WorkspaceSymbolIndex> index =
        std::make_shared<analysis::WorkspaceSymbolIndex>();
    std::string mainUri;

    explicit Workspace(const std::string& mainSource) {
        const std::string circle = "class Circle {}\n";
        dir.createFile("shapes/Circle.mt", circle);
        dir.createFile("main.mt", mainSource);
        index->reindexFile(uriFor("shapes/Circle.mt"), circle);
        mainUri = uriFor("main.mt");
        docMgr.openDocument(mainUri, mainSource, 1);
    }

    std::string uriFor(const std::string& relativePath) const {
        return UriUtils::filePathToUri((fs::path(dir.path()) / relativePath).string());
    }
};

// What an editor holds: the published diagnostics after a JSON round trip,
// keeping only those on `line`, as the mType editor's Quick Fix request does.
std::vector<Diagnostic> publishedOnLine(DocumentManager& docMgr, const std::string& uri, int line) {
    DiagnosticsHandler diagnostics(&docMgr);
    std::vector<Diagnostic> published;
    diagnostics.setPublisher([&](const std::string&, const std::vector<Diagnostic>& diags) {
        published = diags;
    });
    diagnostics.publishDiagnostics(uri);

    std::vector<Diagnostic> onLine;
    for (const auto& d : published) {
        Diagnostic echoed;
        from_json(d.toJson(), echoed);
        if (echoed.range.start.line <= line && line <= echoed.range.end.line) {
            onLine.push_back(std::move(echoed));
        }
    }
    return onLine;
}

int countUnresolved(const std::vector<Diagnostic>& diags) {
    int n = 0;
    for (const auto& d : diags) {
        if (d.data && d.data->value("exceptionType", "") == "ClassNotFoundException") ++n;
    }
    return n;
}

std::vector<const CodeAction*> importActions(const std::vector<CodeAction>& actions) {
    std::vector<const CodeAction*> found;
    for (const auto& a : actions) {
        if (a.title.rfind("Add import 'Circle'", 0) == 0) found.push_back(&a);
    }
    return found;
}

CodeAction actionWith(const std::string& title, const std::string& newText, int diagLine) {
    CodeAction action;
    action.title = title;
    action.kind = "quickfix";
    WorkspaceEdit edit;
    TextEdit textEdit;
    textEdit.newText = newText;
    edit.changes["file:///main.mt"].push_back(textEdit);
    action.edit = edit;
    Diagnostic d;
    d.range = {{diagLine, 0}, {diagLine, 6}};
    d.severity = 1;
    d.message = "use of undeclared type 'Circle'";
    action.diagnostics.push_back(d);
    return action;
}

void registerEndToEndTests(LspTestHarness& harness) {
    harness.addTest("Circle c = new Circle(); without an import offers one Add import", []() {
        Workspace ws("Circle c = new Circle();\n");
        const auto lineDiags = publishedOnLine(ws.docMgr, ws.mainUri, 0);
        require(countUnresolved(lineDiags) == 2,
            "expected unresolved-type diagnostics on the declaration and the new expression, got "
            + std::to_string(countUnresolved(lineDiags)));

        CodeActionHandler handler(&ws.docMgr, ws.index);
        const auto actions = handler.handleCodeAction(ws.mainUri, Range{{0, 0}, {0, 0}}, lineDiags);
        const auto imports = importActions(actions);
        require(imports.size() == 1,
            "expected exactly one Add import action, got " + std::to_string(imports.size()));

        const CodeAction& fix = *imports.front();
        require(fix.title == "Add import 'Circle' from \"./shapes/Circle.mt\"",
            "unexpected title: " + fix.title);
        require(fix.edit && fix.edit->changes.count(ws.mainUri) == 1,
            "the edit must target main.mt");
        require(fix.edit->changes.at(ws.mainUri).front().newText == kCircleImport,
            "unexpected import text: " + fix.edit->changes.at(ws.mainUri).front().newText);
        require(fix.diagnostics.size() == 2, "the merged action should cover both diagnostics");
    });

    harness.addTest("once Circle is imported there is no diagnostic and no fix", []() {
        Workspace ws(kCircleImport + "Circle c = new Circle();\n");
        const auto lineDiags = publishedOnLine(ws.docMgr, ws.mainUri, 1);
        require(countUnresolved(lineDiags) == 0, "an imported Circle must not be flagged");

        CodeActionHandler handler(&ws.docMgr, ws.index);
        const auto actions = handler.handleCodeAction(ws.mainUri, Range{{1, 0}, {1, 0}}, lineDiags);
        require(importActions(actions).empty(), "no Add import once the import exists");
    });

    harness.addTest("data.missingName wins over the word under the range", []() {
        // Range points at "Object"; the producer says the missing type is Circle.
        Workspace ws("Object o = null;\n");
        Diagnostic diag;
        diag.range = {{0, 0}, {0, 6}};
        diag.severity = 1;
        diag.message = "use of undeclared type 'Circle'";
        diag.data = json{{"exceptionType", "ClassNotFoundException"}, {"missingName", "Circle"}};

        CodeActionHandler handler(&ws.docMgr, ws.index);
        const auto actions = handler.handleCodeAction(ws.mainUri, Range{{0, 0}, {0, 0}}, {diag});
        require(importActions(actions).size() == 1, "the fix should import Circle");
    });
}

void registerMergerTests(LspTestHarness& harness) {
    harness.addTest("merger: identical actions collapse and keep both diagnostics", []() {
        const auto merged = CodeActionMerger::mergeDuplicates({
            actionWith("Add import 'Circle'", "import { Circle } from \"a.mt\";\n", 0),
            actionWith("Add import 'Circle'", "import { Circle } from \"a.mt\";\n", 1)});
        require(merged.size() == 1, "expected one action, got " + std::to_string(merged.size()));
        require(merged[0].diagnostics.size() == 2, "both diagnostics should be kept");
    });

    harness.addTest("merger: same title with a different edit stays separate", []() {
        const auto merged = CodeActionMerger::mergeDuplicates({
            actionWith("Add import 'Circle'", "import { Circle } from \"a.mt\";\n", 0),
            actionWith("Add import 'Circle'", "import { Circle } from \"b.mt\";\n", 0)});
        require(merged.size() == 2, "different edits are different fixes");
    });

    harness.addTest("merger: order is kept and a repeated diagnostic isn't doubled", []() {
        const auto merged = CodeActionMerger::mergeDuplicates({
            actionWith("First", "x", 0),
            actionWith("Second", "y", 0),
            actionWith("First", "x", 0)});
        require(merged.size() == 2 && merged[0].title == "First" && merged[1].title == "Second",
            "first occurrence keeps its position");
        require(merged[0].diagnostics.size() == 1, "an identical diagnostic is listed once");
    });
}

} // namespace

void MissingImportEndToEndTestSuite::registerTests(LspTestHarness& harness) {
    registerEndToEndTests(harness);
    registerMergerTests(harness);
}

} // namespace mtype::lsp::test
