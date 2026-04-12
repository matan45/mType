#include "LSPTypesTestSuite.hpp"
#include "../src/utils/LSPTypes.hpp"

namespace mtype::lsp::test {

void LSPTypesTestSuite::registerTests(LspTestHarness& harness) {

    harness.addTest("Position JSON round-trip", []() {
        Position pos{10, 5};
        json j = pos;
        auto pos2 = j.get<Position>();
        require(pos2.line == 10, "line mismatch");
        require(pos2.character == 5, "character mismatch");
    });

    harness.addTest("Range JSON round-trip", []() {
        Range r{{1, 2}, {3, 4}};
        json j = r;
        auto r2 = j.get<Range>();
        require(r2.start.line == 1 && r2.start.character == 2, "start mismatch");
        require(r2.end.line == 3 && r2.end.character == 4, "end mismatch");
    });

    harness.addTest("Location JSON round-trip", []() {
        Location loc;
        loc.uri = "file:///test.mt";
        loc.range = {{0, 0}, {0, 5}};
        json j = loc;
        auto loc2 = j.get<Location>();
        require(loc2.uri == "file:///test.mt", "uri mismatch");
        require(loc2.range.start.line == 0, "range mismatch");
    });

    harness.addTest("Diagnostic round-trip with all optional fields", []() {
        Diagnostic d;
        d.range = {{1, 0}, {1, 10}};
        d.severity = 2;
        d.message = "unused variable";
        d.source = "mType";
        d.code = json("MT-W2001");
        d.tags = {1}; // Unnecessary
        d.data = json{{"exceptionType", "UnusedVariable"}};

        json j = d.toJson();
        Diagnostic d2;
        from_json(j, d2);

        require(d2.severity == 2, "severity mismatch");
        require(d2.message == "unused variable", "message mismatch");
        require(d2.source.has_value() && *d2.source == "mType", "source mismatch");
        require(d2.code.has_value() && d2.code->get<std::string>() == "MT-W2001", "code mismatch");
        require(d2.tags.size() == 1 && d2.tags[0] == 1, "tags mismatch");
        require(d2.data.has_value(), "data should be present");
    });

    harness.addTest("Diagnostic from_json with missing optional fields defaults severity to 1", []() {
        json j = {{"range", {{"start", {{"line", 0}, {"character", 0}}},
                              {"end",   {{"line", 0}, {"character", 1}}}}},
                  {"message", "error"}};
        Diagnostic d;
        from_json(j, d);
        require(d.severity == 1, "expected default severity 1 (Error)");
        require(!d.source.has_value(), "source should be absent");
        require(!d.code.has_value(), "code should be absent");
    });

    harness.addTest("CompletionItem toJson includes additionalTextEdits when non-empty", []() {
        CompletionItem item;
        item.label = "Helper";
        item.kind = 7;
        item.detail = "Auto-import";
        TextEdit te;
        te.range = {{0, 0}, {0, 0}};
        te.newText = "import Helper from \"./Helper\";\n";
        item.additionalTextEdits.push_back(te);

        json j = item.toJson();
        require(j.contains("additionalTextEdits"), "should have additionalTextEdits");
        require(j["additionalTextEdits"].size() == 1, "expected 1 edit");
        require(j["additionalTextEdits"][0]["newText"].get<std::string>().find("import") != std::string::npos,
            "edit should contain import");
    });

    harness.addTest("CompletionItem toJson omits optional fields when absent", []() {
        CompletionItem item;
        item.label = "class";
        item.kind = 14;

        json j = item.toJson();
        require(j["label"] == "class", "label present");
        require(j["kind"] == 14, "kind present");
        require(!j.contains("detail"), "detail should be absent");
        require(!j.contains("documentation"), "documentation should be absent");
        require(!j.contains("insertText"), "insertText should be absent");
        require(!j.contains("additionalTextEdits"), "additionalTextEdits should be absent");
    });

    harness.addTest("CodeAction toJson includes diagnostics array", []() {
        CodeAction action;
        action.title = "Add import";
        action.kind = "quickfix";
        Diagnostic d;
        d.range = {{0, 0}, {0, 5}};
        d.severity = 1;
        d.message = "undefined";
        action.diagnostics.push_back(d);

        json j = action.toJson();
        require(j["title"] == "Add import", "title mismatch");
        require(j.contains("kind") && j["kind"] == "quickfix", "kind mismatch");
        require(j.contains("diagnostics") && j["diagnostics"].size() == 1, "diagnostics mismatch");
    });
}

} // namespace mtype::lsp::test
