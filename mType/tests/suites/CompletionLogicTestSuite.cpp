#include "CompletionLogicTestSuite.hpp"

#include "../../util/EditDistance.hpp"
#include "../../util/ImportScan.hpp"
#include "../../util/TokenExtraction.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace tests::testSuite
{
    using namespace testFramework;
    using services::ScriptAPI;

    namespace
    {
        void require(bool cond, const std::string& msg)
        {
            if (!cond)
            {
                throw std::runtime_error(msg);
            }
        }

        // Mirrors the filter in CompletionHandler::applyFuzzyFilter so
        // the test covers the exact composition (prefix-match short
        // circuit + rustc-style budget + levenshtein). Kept local to
        // the test TU so a future refactor of the real helper doesn't
        // have to thread through a shared predicate.
        bool keepUnderFuzzy(const std::string& typed, const std::string& label)
        {
            if (typed.empty()) return true;
            if (label.rfind(typed, 0) == 0) return true;
            const int budget = std::max(1, static_cast<int>(typed.size()) / 3);
            return util::levenshtein(typed, label, budget) <= budget;
        }
    }

    void CompletionLogicTestSuite::setupTests()
    {
        // ================================================================
        // util::findImportInsertLine
        // ================================================================

        addCallbackTest("findImportInsertLine: empty file returns 0", "",
            [](ScriptAPI&) {
                require(util::findImportInsertLine("") == 0,
                    "empty file should insert at line 0");
            });

        addCallbackTest("findImportInsertLine: file with no imports returns 0", "",
            [](ScriptAPI&) {
                const std::string src = "class Foo {}\nfunction main() {}\n";
                require(util::findImportInsertLine(src) == 0,
                    "no imports should insert at line 0");
            });

        addCallbackTest("findImportInsertLine: single import returns line after", "",
            [](ScriptAPI&) {
                const std::string src = "import a from \"./a\";\nclass Foo {}\n";
                require(util::findImportInsertLine(src) == 1,
                    "one import should insert at line 1");
            });

        addCallbackTest("findImportInsertLine: two imports returns line after last", "",
            [](ScriptAPI&) {
                const std::string src =
                    "import a from \"./a\";\n"
                    "import b from \"./b\";\n"
                    "class Foo {}\n";
                require(util::findImportInsertLine(src) == 2,
                    "two imports should insert at line 2");
            });

        addCallbackTest("findImportInsertLine: imports mixed with comments/blanks", "",
            [](ScriptAPI&) {
                const std::string src =
                    "// MYT-51 test file\n"
                    "\n"
                    "import a from \"./a\";\n"
                    "// a comment between imports\n"
                    "import b from \"./b\";\n"
                    "\n"
                    "class Foo {}\n";
                // Last import is on line 4 (0-based). Insertion goes
                // immediately after it, i.e. line 5.
                require(util::findImportInsertLine(src) == 5,
                    "mixed content should insert after last import line");
            });

        addCallbackTest("findImportInsertLine: leading whitespace is OK", "",
            [](ScriptAPI&) {
                const std::string src = "    import a from \"./a\";\nclass Foo {}\n";
                require(util::findImportInsertLine(src) == 1,
                    "indented import should still be recognized");
            });

        addCallbackTest("findImportInsertLine: only scans first 50 lines", "",
            [](ScriptAPI&) {
                // 60 filler lines, then an import. The scan caps at 50
                // so the import isn't seen and the insert line stays 0.
                std::string src;
                for (int i = 0; i < 60; ++i) src += "// filler\n";
                src += "import late from \"./late\";\n";
                require(util::findImportInsertLine(src) == 0,
                    "imports past line 50 are not seen");
            });

        // ================================================================
        // util::extractIdentifierTokenBefore
        // ================================================================

        addCallbackTest("extractIdentifierTokenBefore: cursor mid-identifier", "",
            [](ScriptAPI&) {
                // Line:    "let foo = prntl"
                // Position: column 15 (one past the last 'l')
                const std::string line = "let foo = prntl";
                auto tok = util::extractIdentifierTokenBefore(line, 15);
                require(tok == "prntl",
                    "expected 'prntl', got '" + tok + "'");
            });

        addCallbackTest("extractIdentifierTokenBefore: cursor at column 0", "",
            [](ScriptAPI&) {
                auto tok = util::extractIdentifierTokenBefore("foo", 0);
                require(tok.empty(),
                    "column 0 should return empty, got '" + tok + "'");
            });

        addCallbackTest("extractIdentifierTokenBefore: cursor on whitespace", "",
            [](ScriptAPI&) {
                // Line:    "foo bar"
                // Position: 4 — on the 'b', one past the space
                // The character immediately before column 4 is ' ',
                // which is not an identifier byte, so expect empty.
                auto tok = util::extractIdentifierTokenBefore("foo bar", 4);
                require(tok.empty(),
                    "whitespace-preceded cursor should return empty, got '"
                    + tok + "'");
            });

        addCallbackTest("extractIdentifierTokenBefore: cursor right after dot", "",
            [](ScriptAPI&) {
                // "foo.": cursor at column 4, previous char is '.',
                // not an identifier byte → empty. The member-access
                // branch in the completion handler takes over from
                // here, so the fuzzy filter shouldn't see this.
                auto tok = util::extractIdentifierTokenBefore("foo.", 4);
                require(tok.empty(),
                    "dot-preceded cursor should return empty, got '"
                    + tok + "'");
            });

        addCallbackTest("extractIdentifierTokenBefore: underscores and digits", "",
            [](ScriptAPI&) {
                auto tok = util::extractIdentifierTokenBefore("my_var_42", 9);
                require(tok == "my_var_42",
                    "expected 'my_var_42', got '" + tok + "'");
            });

        addCallbackTest("extractIdentifierTokenBefore: empty line", "",
            [](ScriptAPI&) {
                auto tok = util::extractIdentifierTokenBefore("", 0);
                require(tok.empty(), "empty line should return empty");
            });

        addCallbackTest("extractIdentifierTokenBefore: out-of-range column clamps", "",
            [](ScriptAPI&) {
                // Column 999 past end of "foo" → clamp to 3, return "foo".
                auto tok = util::extractIdentifierTokenBefore("foo", 999);
                require(tok == "foo",
                    "out-of-range column should clamp, got '" + tok + "'");
            });

        // ================================================================
        // Fuzzy filter predicate (prefix-match short-circuit +
        // rustc-style budget + levenshtein)
        // ================================================================

        addCallbackTest("fuzzyFilter: empty prefix keeps everything", "",
            [](ScriptAPI&) {
                require(keepUnderFuzzy("", "anything"),
                    "empty prefix must keep all candidates");
                require(keepUnderFuzzy("", ""),
                    "empty prefix + empty label must keep");
            });

        addCallbackTest("fuzzyFilter: prefix match is kept", "",
            [](ScriptAPI&) {
                require(keepUnderFuzzy("pri", "println"),
                    "prefix match should always be kept");
                require(keepUnderFuzzy("pri", "print"),
                    "prefix match on shorter label should be kept");
            });

        addCallbackTest("fuzzyFilter: typo within budget is kept", "",
            [](ScriptAPI&) {
                // "prntln" → budget = max(1, 6/3) = 2,
                // levenshtein("prntln", "println") = 1 → keep.
                require(keepUnderFuzzy("prntln", "println"),
                    "prntln should fuzzy-match println");
            });

        addCallbackTest("fuzzyFilter: typo outside budget is dropped", "",
            [](ScriptAPI&) {
                // "xyz" (len 3) → budget 1, vs "println" far too distant.
                require(!keepUnderFuzzy("xyz", "println"),
                    "xyz should not match println");
            });

        addCallbackTest("fuzzyFilter: short prefix has minimum budget 1", "",
            [](ScriptAPI&) {
                // len 2 → max(1, 2/3) = max(1, 0) = 1.
                // "fo" vs "foo" is distance 1 → keep.
                require(keepUnderFuzzy("fo", "foo"),
                    "'fo' should match 'foo' within minimum budget");
                // "fo" vs "xyzzy" is distance >> 1 → drop.
                require(!keepUnderFuzzy("fo", "xyzzy"),
                    "'fo' should not match 'xyzzy'");
            });

        addCallbackTest("fuzzyFilter: longer query grows budget proportionally", "",
            [](ScriptAPI&) {
                // len 9 → budget 3. Two substitutions should pass.
                require(keepUnderFuzzy("identifier", "idantifiar"),
                    "two-edit distance should fit length-9 budget");
                // Three substitutions (at budget 3) should also pass.
                require(keepUnderFuzzy("identifier", "idantifiir"),
                    "three-edit distance should still fit budget 3");
            });
    }
}
