#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace mtype::lsp::utils
{
    enum class SegmentKind : std::uint8_t { Code, String, Comment };

    // Lexical region at a line boundary.
    enum class Region : std::uint8_t { Code, String, BlockComment };

    struct Segment
    {
        SegmentKind kind;
        std::size_t start;
        std::size_t length;
    };

    struct LineScan
    {
        std::vector<Segment> segments;  // adjacent, in order, covering the whole line
        Region startsIn = Region::Code;
        Region endsIn = Region::Code;
    };

    /**
     * Splits source lines into code, string-literal and comment segments,
     * carrying lexical state from one line to the next, because block
     * comments and string literals may span lines.
     *
     * Mirrors the core lexer (mType/lexer/Lexer.cpp, Lexer_Tokens.cpp):
     * double-quoted strings where a backslash escapes the next character;
     * interpolated `$"..."` strings whose `{...}` holes are code that may hold
     * nested strings, comments and braces; `//` line comments; and block
     * comments, which do not nest. An interpolated string is reported as ONE
     * String segment, holes included. There are no single-quoted literals.
     *
     * Use one instance per pass over a document and feed it every line in
     * order, without the line terminator.
     */
    class SourceSegmenter
    {
    public:
        LineScan scanLine(std::string_view line);
        Region region() const;

    private:
        enum class Frame : std::uint8_t { String, InterpText, InterpExpr, BlockComment };

        struct Entry
        {
            Frame frame;
            int braceDepth = 0;
        };

        SegmentKind insideKind() const;
        std::size_t stepCode(std::string_view line, std::size_t i, SegmentKind& kind);
        std::size_t stepInside(std::string_view line, std::size_t i);
        std::size_t stepString(std::string_view line, std::size_t i);
        std::size_t stepHole(std::string_view line, std::size_t i);

        std::vector<Entry> stack_;  // empty == plain code
    };

    // First / last non-blank character inside the line's Code segments, or '\0'.
    char firstCodeChar(std::string_view line, const LineScan& scan);
    char lastCodeChar(std::string_view line, const LineScan& scan);
}
