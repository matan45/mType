#include "LspDiagnosticConverterTestSuite.hpp"
#include "../src/utils/LspDiagnosticConverter.hpp"
#include "../src/utils/LSPTypes.hpp"

namespace mtype::lsp::test {

namespace {

// Helper to build a core diagnostic with a source location.
diagnostics::Diagnostic makeCoreDiag(const std::string& message,
                                      const std::string& filename,
                                      int line, int col,
                                      int endLine = 0, int endCol = 0) {
    diagnostics::Diagnostic d;
    d.message = message;
    d.severity = diagnostics::Severity::Error;
    d.primarySpan.start = errors::SourceLocation(filename, line, col);
    d.primarySpan.end = errors::SourceLocation(filename,
        endLine > 0 ? endLine : line,
        endCol > 0 ? endCol : col);
    return d;
}

} // anonymous namespace

void LspDiagnosticConverterTestSuite::registerTests(LspTestHarness& harness) {

    harness.addTest("line/column mapping: 1-based to 0-based", []() {
        auto core = makeCoreDiag("error", "test.mt", 5, 10, 5, 15);
        auto lsp = toLspDiagnostic(core, "file:///test.mt");
        require(lsp.range.start.line == 4, "start line should be 4 (5-1)");
        require(lsp.range.start.character == 9, "start char should be 9 (10-1)");
        require(lsp.range.end.line == 4, "end line should be 4");
        require(lsp.range.end.character == 14, "end char should be 14 (15-1)");
    });

    harness.addTest("point span expands to single character", []() {
        auto core = makeCoreDiag("error", "test.mt", 3, 7);
        // end == start → point span
        auto lsp = toLspDiagnostic(core, "file:///test.mt");
        require(lsp.range.start.line == 2, "start line");
        require(lsp.range.start.character == 6, "start char");
        require(lsp.range.end.line == 2, "end line same as start");
        require(lsp.range.end.character == 7, "end char = start+1 for point span");
    });

    harness.addTest("severity passthrough", []() {
        diagnostics::Diagnostic core;
        core.message = "warning";
        core.severity = diagnostics::Severity::Warning;
        core.primarySpan.start = errors::SourceLocation("test.mt", 1, 1);
        core.primarySpan.end = core.primarySpan.start;

        auto lsp = toLspDiagnostic(core, "file:///test.mt");
        require(lsp.severity == static_cast<int>(diagnostics::Severity::Warning),
            "severity should match Warning");
    });

    harness.addTest("source is set to mType", []() {
        auto core = makeCoreDiag("err", "test.mt", 1, 1);
        auto lsp = toLspDiagnostic(core, "file:///test.mt");
        require(lsp.source.has_value() && *lsp.source == "mType",
            "source should be 'mType'");
    });

    harness.addTest("exceptionType appears in data blob", []() {
        auto core = makeCoreDiag("undefined", "test.mt", 1, 1);
        core.sourceExceptionType = "ClassNotFoundException";

        auto lsp = toLspDiagnostic(core, "file:///test.mt");
        require(lsp.data.has_value(), "data blob should be present");
        require((*lsp.data)["exceptionType"] == "ClassNotFoundException",
            "exceptionType mismatch");
    });

    harness.addTest("suggestions appear in data blob", []() {
        auto core = makeCoreDiag("undefined var", "test.mt", 1, 1);
        diagnostics::Suggestion s;
        s.label = "did you mean 'foo'?";
        s.renderedHint = "foo";
        core.suggestions.push_back(s);

        auto lsp = toLspDiagnostic(core, "file:///test.mt");
        require(lsp.data.has_value(), "data should be present");
        require((*lsp.data).contains("suggestions"), "suggestions key missing");
        require((*lsp.data)["suggestions"].size() == 1, "expected 1 suggestion");
    });

    harness.addTest("no-location fallback to 0:0-0:1", []() {
        diagnostics::Diagnostic core;
        core.message = "no location error";
        core.severity = diagnostics::Severity::Error;
        // Default SourceLocation has filename "<unknown>" → hasLocation() = false

        auto lsp = toLspDiagnostic(core, "file:///test.mt");
        require(lsp.range.start.line == 0 && lsp.range.start.character == 0,
            "start should be 0:0");
        require(lsp.range.end.line == 0 && lsp.range.end.character == 1,
            "end should be 0:1");
    });

    // MYT-364: a Suggestion with structured TextEdits must round-trip
    // through the JSON `data.suggestions[*].edits` payload so the LSP
    // code-action handler can apply them verbatim. Critically, zero-width
    // inserts (start == end) must NOT be widened — the legacy
    // `toLspRange` helper widens point spans for squiggle visibility,
    // but applying that widening to a structured edit would turn an
    // "insert ';'" into a 1-character REPLACE over the next byte, which
    // is exactly the MYT-364 corruption.
    harness.addTest("suggestion edits round-trip with zero-width preserved", []() {
        auto core = makeCoreDiag("expected ';'", "test.mt", 5, 10);
        diagnostics::Suggestion s;
        s.label = "insert ';'";
        // Zero-width insert at line 5, column 10 (1-based) →
        // LSP (line 4, character 9), end == start.
        diagnostics::TextEdit te;
        te.start = errors::SourceLocation("test.mt", 5, 10);
        te.end = errors::SourceLocation("test.mt", 5, 10);
        te.newText = ";";
        s.edits.push_back(te);
        s.applicability = diagnostics::FixApplicability::MachineApplicable;
        core.suggestions.push_back(std::move(s));

        auto lsp = toLspDiagnostic(core, "file:///test.mt");
        require(lsp.data.has_value(), "data should be present");
        require((*lsp.data).contains("suggestions"), "suggestions key missing");
        const auto& sug = (*lsp.data)["suggestions"][0];
        require(sug.contains("edits") && sug["edits"].is_array(),
            "edits array should be serialized");
        require(sug["edits"].size() == 1, "expected exactly one edit");
        const auto& ej = sug["edits"][0];
        require(ej["start"]["line"] == 4,
            "edit start line should be 4 (5-1)");
        require(ej["start"]["character"] == 9,
            "edit start character should be 9 (10-1)");
        require(ej["end"]["line"] == 4,
            "edit end line should be 4 (5-1)");
        require(ej["end"]["character"] == 9,
            "edit end character MUST equal start (zero-width insert preserved, "
            "not widened by toLspRange) — MYT-364 regression");
        require(ej["newText"] == ";", "newText should be ';'");
        require(sug["applicability"]
                == static_cast<int>(diagnostics::FixApplicability::MachineApplicable),
            "applicability should round-trip as MachineApplicable");
    });

    harness.addTest("notes become relatedInformation entries", []() {
        auto core = makeCoreDiag("error", "test.mt", 1, 1, 1, 5);
        core.notes.push_back("consider adding a type annotation");

        auto lsp = toLspDiagnostic(core, "file:///test.mt");
        require(lsp.relatedInformation.size() == 1,
            "expected 1 relatedInformation from note");
        require(lsp.relatedInformation[0].message == "consider adding a type annotation",
            "note message mismatch");
    });
}

} // namespace mtype::lsp::test
