#include "FormattingHandlerTestSuite.hpp"
#include "../src/handlers/FormattingHandler.hpp"

namespace mtype::lsp::test {

void FormattingHandlerTestSuite::registerTests(LspTestHarness& harness) {

    // formatDocument always returns a single full-document replacement edit.
    // It never returns an empty vector.

    harness.addTest("single-line: no indent change", []() {
        FormattingHandler handler;
        FormattingOptions opts;
        opts.tabSize = 4;
        opts.insertSpaces = true;
        opts.insertFinalNewline = false;

        auto edits = handler.formatDocument("let x: int = 5;", opts);
        require(edits.size() == 1, "expected 1 full-document edit");
        require(edits[0].range.start.line == 0 && edits[0].range.start.character == 0,
            "edit should start at 0:0");
        require(edits[0].newText.find("let x") != std::string::npos,
            "output should contain the statement");
    });

    harness.addTest("brace indentation: content after { is indented", []() {
        FormattingHandler handler;
        FormattingOptions opts;
        opts.tabSize = 4;
        opts.insertSpaces = true;
        opts.insertFinalNewline = false;

        std::string source = "class Foo {\nfunction bar(): void {}\n}";
        auto edits = handler.formatDocument(source, opts);

        require(edits.size() == 1, "expected 1 edit");
        // Line after opening brace should have 4-space indent
        require(edits[0].newText.find("\n    ") != std::string::npos,
            "expected 4-space indent after opening brace");
    });

    harness.addTest("insertSpaces=true with tabSize=2 produces 2-space indent", []() {
        FormattingHandler handler;
        FormattingOptions opts;
        opts.tabSize = 2;
        opts.insertSpaces = true;
        opts.insertFinalNewline = false;

        std::string source = "class Foo {\nfunction bar(): void {}\n}";
        auto edits = handler.formatDocument(source, opts);

        require(edits.size() == 1, "expected 1 edit");
        // With tabSize=2, indent should be 2 spaces (not 4)
        std::string text = edits[0].newText;
        require(text.find("\n  ") != std::string::npos, "expected 2-space indent");
    });

    harness.addTest("insertSpaces=false produces tab indent", []() {
        FormattingHandler handler;
        FormattingOptions opts;
        opts.tabSize = 4;
        opts.insertSpaces = false;
        opts.insertFinalNewline = false;

        std::string source = "class Foo {\nfunction bar(): void {}\n}";
        auto edits = handler.formatDocument(source, opts);

        require(edits.size() == 1, "expected 1 edit");
        require(edits[0].newText.find('\t') != std::string::npos,
            "expected tab character in formatted output");
    });

    harness.addTest("trimTrailingWhitespace removes trailing spaces", []() {
        FormattingHandler handler;
        FormattingOptions opts;
        opts.trimTrailingWhitespace = true;
        opts.insertFinalNewline = true;

        std::string source = "let x: int = 5;   \n";
        auto edits = handler.formatDocument(source, opts);

        require(edits.size() == 1, "expected 1 edit");
        // Check no line ends with spaces before a newline
        std::string text = edits[0].newText;
        for (size_t i = 1; i < text.size(); ++i) {
            if (text[i] == '\n') {
                require(text[i - 1] != ' ' && text[i - 1] != '\t',
                    "trailing whitespace should be trimmed");
            }
        }
    });

    harness.addTest("insertFinalNewline adds trailing newline", []() {
        FormattingHandler handler;
        FormattingOptions opts;
        opts.insertFinalNewline = true;

        std::string source = "let x: int = 5;";
        auto edits = handler.formatDocument(source, opts);

        require(edits.size() == 1, "expected 1 edit");
        require(!edits[0].newText.empty() && edits[0].newText.back() == '\n',
            "formatted output should end with newline");
    });

    harness.addTest("operator spacing: a=b becomes a = b", []() {
        FormattingHandler handler;
        FormattingOptions opts;
        opts.insertFinalNewline = false;

        // formatOperators uses regex: (\w)=(\w) → \1 = \2
        std::string source = "let x=5;";
        auto edits = handler.formatDocument(source, opts);

        require(edits.size() == 1, "expected 1 edit");
        require(edits[0].newText.find("x = 5") != std::string::npos,
            "expected spaces around = operator");
    });

    harness.addTest("comma spacing: a,b becomes a, b", []() {
        FormattingHandler handler;
        FormattingOptions opts;
        opts.insertFinalNewline = false;

        // formatOperators uses regex: ,(\S) → , \1
        std::string source = "function foo(a: int,b: int): void {}";
        auto edits = handler.formatDocument(source, opts);

        require(edits.size() == 1, "expected 1 edit");
        require(edits[0].newText.find(", b") != std::string::npos,
            "expected space after comma");
    });

    harness.addTest("formatRange delegates to formatDocument", []() {
        FormattingHandler handler;
        FormattingOptions opts;

        std::string source = "class Foo {\nfunction bar(): void {}\n}\n";
        Range range{{1, 0}, {1, 30}};

        // Current implementation: formatRange just calls formatDocument
        auto rangeEdits = handler.formatRange(source, range, opts);
        auto fullEdits = handler.formatDocument(source, opts);

        require(rangeEdits.size() == fullEdits.size(), "same number of edits");
        if (!rangeEdits.empty() && !fullEdits.empty()) {
            require(rangeEdits[0].newText == fullEdits[0].newText,
                "same output text");
        }
    });
}

} // namespace mtype::lsp::test
