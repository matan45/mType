#include "SourceSegmenterTestSuite.hpp"
#include "../src/utils/SourceSegmenter.hpp"

#include <string>
#include <string_view>

namespace mtype::lsp::test {

namespace {

using utils::LineScan;
using utils::Region;
using utils::SegmentKind;
using utils::SourceSegmenter;

const char* kindName(SegmentKind kind) {
    switch (kind) {
        case SegmentKind::Code: return "code";
        case SegmentKind::String: return "str";
        case SegmentKind::Comment: return "cmt";
    }
    return "?";
}

const char* regionName(Region region) {
    switch (region) {
        case Region::Code: return "Code";
        case Region::String: return "String";
        case Region::BlockComment: return "BlockComment";
    }
    return "?";
}

// Renders a scan as e.g. `code[x = ]str["a"]code[;]` for readable assertions.
std::string describe(std::string_view line, const LineScan& scan) {
    std::string out;
    for (const auto& segment : scan.segments) {
        out += kindName(segment.kind);
        out += '[';
        out += line.substr(segment.start, segment.length);
        out += ']';
    }
    return out;
}

void expectScan(SourceSegmenter& segmenter, std::string_view line,
                const std::string& expected, Region expectedEnd = Region::Code) {
    const LineScan scan = segmenter.scanLine(line);
    const std::string actual = describe(line, scan);
    require(actual == expected, "scan of `" + std::string(line) + "`\n         got:      "
        + actual + "\n         expected: " + expected);
    require(scan.endsIn == expectedEnd && segmenter.region() == expectedEnd,
        "`" + std::string(line) + "` should end in " + regionName(expectedEnd)
        + ", ended in " + regionName(scan.endsIn));
}

} // namespace

void SourceSegmenterTestSuite::registerTests(LspTestHarness& harness) {
    harness.addTest("plain code is one code segment", []() {
        SourceSegmenter segmenter;
        expectScan(segmenter, "int x = 1;", "code[int x = 1;]");
    });

    harness.addTest("backslash escapes inside a string literal", []() {
        SourceSegmenter segmenter;
        expectScan(segmenter, R"mt(a = "x\"y\\"; b)mt", R"mt(code[a = ]str["x\"y\\"]code[; b])mt");
    });

    harness.addTest("line comment runs to end of line and ignores quotes", []() {
        SourceSegmenter segmenter;
        expectScan(segmenter, R"mt(x; // don't "q)mt", R"mt(code[x; ]cmt[// don't "q])mt");
    });

    harness.addTest("apostrophes are not quotes", []() {
        SourceSegmenter segmenter;
        expectScan(segmenter, "it's x", "code[it's x]");
    });

    harness.addTest("block comment spans lines and does not nest", []() {
        SourceSegmenter segmenter;
        expectScan(segmenter, "a /* b /* c", "code[a ]cmt[/* b /* c]", Region::BlockComment);
        const LineScan next = segmenter.scanLine("d */ e");
        require(next.startsIn == Region::BlockComment, "second line should start inside the comment");
        require(describe("d */ e", next) == "cmt[d */]code[ e]", "got " + describe("d */ e", next));
    });

    harness.addTest("the star of /* does not close the comment", []() {
        SourceSegmenter segmenter;
        expectScan(segmenter, "/*/ x", "cmt[/*/ x]", Region::BlockComment);
    });

    harness.addTest("string literal spans lines", []() {
        SourceSegmenter segmenter;
        expectScan(segmenter, R"mt(s = "ab)mt", R"mt(code[s = ]str["ab])mt", Region::String);
        const LineScan next = segmenter.scanLine(R"mt(cd" + x)mt");
        require(next.startsIn == Region::String, "second line should start inside the string");
        require(describe(R"mt(cd" + x)mt", next) == R"mt(str[cd"]code[ + x])mt",
            "got " + describe(R"mt(cd" + x)mt", next));
    });

    harness.addTest("interpolated string with nested string, braces and comment is one segment", []() {
        SourceSegmenter segmenter;
        expectScan(segmenter, R"mt(v = $"a {f("}", {1}) /* } */} b" + c)mt",
            R"mt(code[v = ]str[$"a {f("}", {1}) /* } */} b"]code[ + c])mt");
    });

    harness.addTest("escaped braces do not open an interpolation hole", []() {
        SourceSegmenter segmenter;
        expectScan(segmenter, R"mt($"\{x\}" y)mt", R"mt(str[$"\{x\}"]code[ y])mt");
    });

    harness.addTest("nested interpolated strings", []() {
        SourceSegmenter segmenter;
        expectScan(segmenter, R"mt($"o {$"i {"q"}"} t"; z)mt", R"mt(str[$"o {$"i {"q"}"} t"]code[; z])mt");
    });

    harness.addTest("unterminated string keeps later lines inside it", []() {
        SourceSegmenter segmenter;
        expectScan(segmenter, R"mt(x = "abc)mt", R"mt(code[x = ]str["abc])mt", Region::String);
        expectScan(segmenter, "int y=1;", "str[int y=1;]", Region::String);
    });

    harness.addTest("first and last code characters skip strings and comments", []() {
        SourceSegmenter segmenter;
        const std::string line = R"mt(} x = "{"; // {)mt";
        const LineScan scan = segmenter.scanLine(line);
        require(utils::firstCodeChar(line, scan) == '}', "first code character should be '}'");
        require(utils::lastCodeChar(line, scan) == ';', "last code character should be ';'");

        const std::string commentOnly = "   // only a comment {";
        const LineScan commentScan = segmenter.scanLine(commentOnly);
        require(utils::lastCodeChar(commentOnly, commentScan) == '\0', "a comment-only line has no code");
    });
}

} // namespace mtype::lsp::test
