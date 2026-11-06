#include "FormattingHandlerTestSuite.hpp"
#include "FormattingTestHelpers.hpp"
#include "../src/handlers/FormattingHandler.hpp"

#include <sstream>

namespace mtype::lsp::test {

namespace {

using formatting::FormatCase;
using formatting::addFormatCases;
using formatting::expectFormatted;
using formatting::unchanged;

// MYT-443: every word the old keyword rule spaced. `foreach` is not an mType
// keyword and `from` is used as a variable name, but the old rule split
// identifiers that started with any of them (forEach -> "for Each").
const std::vector<std::string> kWordsTheOldRuleSpaced = {
    "class", "interface", "function", "if", "else", "while",
    "for", "foreach", "return", "new", "import", "from",
    "public", "private", "protected", "static", "final",
    "abstract", "async", "await"
};

void registerKeywordPrefixTests(LspTestHarness& harness) {
    for (const auto& word : kWordsTheOldRuleSpaced) {
        harness.addTest("keyword prefix: identifiers around '" + word + "' are untouched", [word]() {
            for (const std::string& line : {word + "Tail(1);", word + "2(1);", "my" + word + "(1);"}) {
                expectFormatted(line, line);
            }
        });
    }
}

void registerTicketExampleTests(LspTestHarness& harness) {
    addFormatCases(harness, "MYT-443 example", {
        unchanged("forEach", "list.forEach(x -> print(x));"),
        unchanged("format", "string s = format(x);"),
        unchanged("newValue", "int newValue = 1;"),
        unchanged("finally", "} finally {"),
        unchanged("className", "string className = \"A\";"),
        unchanged("ifPresent", "opt.ifPresent(x -> print(x));"),
        unchanged("returnType", "Type returnType = t;"),
        unchanged("Class::forName", "Class::forName(\"A\");"),
        unchanged("bare return", "return;"),
        unchanged("foreach is not a keyword", "foreach(item);"),
        unchanged("from used as a name", "from(x);"),
    });
}

void registerKeywordSpacingTests(LspTestHarness& harness) {
    addFormatCases(harness, "keyword spacing", {
        {"if(", "if(x){", "if (x){"},
        {"while(", "while(i < n){", "while (i < n){"},
        {"for(", "for(int i = 0; i < n; i++){", "for (int i = 0; i < n; i++){"},
        {"else{", "else{", "else {"},
        {"}else{", "}else{", "}else {"},
        {"else if(", "} else if(x){", "} else if (x){"},
        {"return(", "return(x);", "return (x);"},
        unchanged("already spaced", "if (x) {"),
    });
}

void registerOperatorSpacingTests(LspTestHarness& harness) {
    addFormatCases(harness, "operator spacing", {
        {"assignment", "let x=5;", "let x = 5;"},
        {"chained assignment in one pass", "a=b=c;", "a = b = c;"},
        {"comma", "function foo(a: int,b: int): void {}", "function foo(a: int, b: int): void {}"},
        {"consecutive commas in one pass", "f(a,,b);", "f(a, , b);"},
        {"type annotation colon", "function f(x:int): void {}", "function f(x: int): void {}"},
        {"generic arguments", "Map<string,int> m;", "Map<string, int> m;"},
        unchanged("equality", "if (x==y) {"),
        unchanged("scope operator", "Audio::play();"),
        {"scope operator with arguments", "Audio::play(a,b);", "Audio::play(a, b);"},
        {"space after scope operator removed", "Audio:: play();", "Audio::play();"},
        unchanged("scope operator in expression", "int a = Config::B;"),
        {"comma before string", "print(a,\"b\");", "print(a, \"b\");"},
        unchanged("assignment of string", "x=\"a\";"),
    });
}

void registerStringLiteralTests(LspTestHarness& harness) {
    addFormatCases(harness, "strings untouched", {
        unchanged("operators and commas", R"mt(string s = "a=b,c";)mt"),
        unchanged("colon", R"mt(string s = "x:int";)mt"),
        unchanged("scope operator with space", R"mt(string s = "a:: b";)mt"),
        unchanged("keywords and keyword prefixes", R"mt(string s = "if(x) else{ format newValue finally";)mt"),
        unchanged("method name lookup", R"mt(getMethod("staticAdd", 2);)mt"),
        unchanged("keyword before closing quote", R"mt(print("no parent to override from");)mt"),
        unchanged("interpolation text", R"mt(string s = $"a={x},b={y}";)mt"),
        unchanged("string inside interpolation hole", R"mt(string s = $"v: {f("a,b")}";)mt"),
        unchanged("escaped interpolation braces", R"mt(string s = $"\{a=b\}";)mt"),
        unchanged("comment inside interpolation hole", R"mt(string s = $"{x /* } */ }a=b";)mt"),
        unchanged("nested interpolation", R"mt(string s = $"o {$"i {"q,r"}"} t";)mt"),
    });
    addFormatCases(harness, "code after strings", {
        {"escaped backslash", R"mt(string s = "C:\\"; int y=2;)mt", R"mt(string s = "C:\\"; int y = 2;)mt"},
        {"escaped quote", R"mt(string s = "say \"a=b\" now"; int y=2;)mt", R"mt(string s = "say \"a=b\" now"; int y = 2;)mt"},
        {"url", R"mt(string u = "http://h/p?a=b,c"; int y=2;)mt", R"mt(string u = "http://h/p?a=b,c"; int y = 2;)mt"},
        {"comment opener", R"mt(string g = "/*"; int z=3;)mt", R"mt(string g = "/*"; int z = 3;)mt"},
        {"apostrophe", R"mt(print("it's"); x=1;)mt", R"mt(print("it's"); x = 1;)mt"},
    });
}

void registerCommentTests(LspTestHarness& harness) {
    addFormatCases(harness, "comments untouched", {
        unchanged("line comment", "// a=b,c x:int format if(x) Audio:: play"),
        {"trailing line comment", "int x=1; // don't a=b,c", "int x = 1; // don't a=b,c"},
        {"quote inside line comment", "// say \"hi\nint x=1;", "// say \"hi\nint x = 1;"},
        {"inline block comment", "/* a=b,c */ int x=1;", "/* a=b,c */ int x = 1;"},
        {"block comment between statements", "int x=1; /* a=b */ int y=2;", "int x = 1; /* a=b */ int y = 2;"},
        {"quote inside block comment", "/* \"open */ int x=1;", "/* \"open */ int x = 1;"},
        {"block comments do not nest", "/* a /* b */ int x=1;", "/* a /* b */ int x = 1;"},
        {"multi-line block comment",
            "/*\na=b,c:d\nif(x) format newValue\n*/\nint x=1;",
            "/*\na=b,c:d\nif(x) format newValue\n*/\nint x = 1;"},
        {"block comment spanning statements", "int x=1; /* a=b\nc=d */ int y=2;", "int x = 1; /* a=b\nc=d */ int y = 2;"},
        {"division is not a comment", "int r = a/b; int y=2;", "int r = a/b; int y = 2;"},
        {"doc comment body kept verbatim",
            "class A {\n    /**\n     * a=b,c\n     */\nint x=1;\n}",
            "class A {\n    /**\n     * a=b,c\n     */\n    int x = 1;\n}"},
    });
}

void registerLayoutTests(LspTestHarness& harness) {
    addFormatCases(harness, "layout", {
        {"brace before trailing comment indents",
            "class A {\nif (x) { // c\nfoo();\n}\n}",
            "class A {\n    if (x) { // c\n        foo();\n    }\n}"},
        {"brace inside comment does not indent",
            "class A {\nfoo(); // {\nbar();\n}",
            "class A {\n    foo(); // {\n    bar();\n}"},
        {"brace inside string does not indent", "string s = \"{\";\nint y=1;", "string s = \"{\";\nint y = 1;"},
        {"multi-line string kept verbatim",
            "class A {\n    string s = \"line1   \n  a=b, c  \n\n   \n end\";\nint y=1;\n}",
            "class A {\n    string s = \"line1   \n  a=b, c  \n\n   \n end\";\n    int y = 1;\n}"},
        {"CRLF input becomes LF", "int x=1;\r\nint y=2;\r\n", "int x = 1;\nint y = 2;\n"},
        {"trailing blank line kept", "import * from \"a.mt\";\nmain();\n\n", "import * from \"a.mt\";\n\nmain();\n\n"},
    });
    harness.addTest("layout: no newline appended inside an unterminated string", []() {
        expectFormatted("msg = \"unterminated a=b", "msg = \"unterminated a=b", FormattingOptions{});
    });
    harness.addTest("layout: no newline appended inside an unclosed block comment", []() {
        expectFormatted("int x=1; /* unclosed a=b", "int x = 1; /* unclosed a=b", FormattingOptions{});
    });
    harness.addTest("layout: 100k-character lines", []() {
        const std::string filler(100000, 'a');
        expectFormatted("int x=1; // " + filler, "int x = 1; // " + filler);
        expectFormatted("string s = \"" + filler + "\"; int y=2;", "string s = \"" + filler + "\"; int y = 2;");
    });
}

void registerImportTests(LspTestHarness& harness) {
    addFormatCases(harness, "imports", {
        {"sorted with one blank line",
            "import * from \"b.mt\";\nimport * from \"a.mt\";\nint x = 1;",
            "import * from \"a.mt\";\nimport * from \"b.mt\";\n\nint x = 1;"},
        {"existing blank line not duplicated",
            "import * from \"b.mt\";\nimport * from \"a.mt\";\n\nint x = 1;",
            "import * from \"a.mt\";\nimport * from \"b.mt\";\n\nint x = 1;"},
        {"import without space before brace",
            "import{a} from \"x.mt\";\nint y=1;",
            "import {a} from \"x.mt\";\n\nint y = 1;"},
        {"multi-line import left in place",
            "import * from \"b.mt\";\nimport {\n    A\n} from \"z.mt\";\nint y=1;",
            "import * from \"b.mt\";\n\nimport {\n    A\n} from \"z.mt\";\nint y = 1;"},
        {"trailing comment on import line",
            "import * from \"b.mt\"; // why\nimport * from \"a.mt\";\nmain();",
            "import * from \"a.mt\";\nimport * from \"b.mt\"; // why\n\nmain();"},
        {"duplicate imports do not duplicate code",
            "import * from \"a.mt\";\nimport * from \"a.mt\";\nimport * from \"a.mt\";\nmain();\n",
            "import * from \"a.mt\";\n\nmain();\n"},
    });
}

} // namespace

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
        // Split into lines and check that line after "class Foo {" has 4-space indent
        std::istringstream ss(edits[0].newText);
        std::string line;
        bool foundIndented = false;
        bool afterBrace = false;
        while (std::getline(ss, line)) {
            if (afterBrace && !line.empty()) {
                require(line.substr(0, 4) == "    ",
                    "line after opening brace should start with 4 spaces, got: '" + line + "'");
                foundIndented = true;
                break;
            }
            if (line.find('{') != std::string::npos) afterBrace = true;
        }
        require(foundIndented, "should have found an indented line after opening brace");
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

        // '=' between two word characters gets a space on each side.
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

        // A comma followed by a non-space character gets a space after it.
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

    // MYT-443 regressions: exact output, always checked for idempotency.
    registerTicketExampleTests(harness);
    registerKeywordPrefixTests(harness);
    registerKeywordSpacingTests(harness);
    registerOperatorSpacingTests(harness);
    registerStringLiteralTests(harness);
    registerCommentTests(harness);
    registerLayoutTests(harness);
    registerImportTests(harness);
}

} // namespace mtype::lsp::test
