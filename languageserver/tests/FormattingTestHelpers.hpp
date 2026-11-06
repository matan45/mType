#pragma once

#include "LspTestHarness.hpp"
#include "../src/handlers/FormattingHandler.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

// Shared helpers for the formatting suites (MYT-443). Every check goes through
// applyEdits() so tests observe exactly what an editor would write back,
// including the edit range: a range that is too short leaves the original tail
// behind and duplicates code.
namespace mtype::lsp::test::formatting {

struct FormatCase {
    std::string name;
    std::string input;
    std::string expected;
};

// A case whose input is already formatted and must come back byte-for-byte.
inline FormatCase unchanged(std::string name, const std::string& text) {
    return FormatCase{std::move(name), text, text};
}

// Byte offset of an LSP position, clamped the way editors clamp positions
// that run past the end of a line or past the last line.
inline std::size_t offsetOf(const std::string& text, const Position& position) {
    std::size_t lineStart = 0;
    for (int line = 0; line < position.line; ++line) {
        std::size_t newline = text.find('\n', lineStart);
        if (newline == std::string::npos) {
            return text.size();
        }
        lineStart = newline + 1;
    }
    std::size_t lineEnd = text.find('\n', lineStart);
    if (lineEnd == std::string::npos) {
        lineEnd = text.size();
    }
    return std::min(lineStart + static_cast<std::size_t>(position.character), lineEnd);
}

// Apply non-overlapping edits back to front, as LSP clients do.
inline std::string applyEdits(const std::string& original, std::vector<TextEdit> edits) {
    std::sort(edits.begin(), edits.end(), [&](const TextEdit& a, const TextEdit& b) {
        return offsetOf(original, a.range.start) > offsetOf(original, b.range.start);
    });
    std::string result = original;
    for (const auto& edit : edits) {
        std::size_t start = offsetOf(original, edit.range.start);
        std::size_t end = std::max(start, offsetOf(original, edit.range.end));
        result.replace(start, end - start, edit.newText);
    }
    return result;
}

// Exact-match defaults: no final newline, so expectations read naturally.
inline FormattingOptions exactOptions() {
    FormattingOptions options;
    options.insertFinalNewline = false;
    return options;
}

inline std::string formatViaEdits(const std::string& source,
                                  const FormattingOptions& options = exactOptions()) {
    FormattingHandler handler;
    return applyEdits(source, handler.formatDocument(source, options));
}

// Render control characters so failure messages show the real difference.
inline std::string visible(const std::string& text) {
    constexpr std::size_t kMaxShown = 240;
    std::string out = "\"";
    for (std::size_t i = 0; i < text.size() && i < kMaxShown; ++i) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20) {
            char buffer[8];
            std::snprintf(buffer, sizeof(buffer), "\\x%02X", c);
            out += buffer;
        } else out += static_cast<char>(c);
    }
    if (text.size() > kMaxShown) out += "...(" + std::to_string(text.size()) + " bytes)";
    return out + "\"";
}

// Formatting `input` must give exactly `expected`, and formatting that output
// again must change nothing (idempotency).
inline void expectFormatted(const std::string& input, const std::string& expected,
                            const FormattingOptions& options = exactOptions()) {
    std::string once = formatViaEdits(input, options);
    require(once == expected,
        "input:    " + visible(input) + "\n         got:      " + visible(once)
        + "\n         expected: " + visible(expected));
    std::string twice = formatViaEdits(once, options);
    require(twice == once,
        "not idempotent for " + visible(input) + "\n         pass 1: " + visible(once)
        + "\n         pass 2: " + visible(twice));
}

inline void addFormatCases(LspTestHarness& harness, const std::string& group,
                           const std::vector<FormatCase>& cases) {
    for (const auto& c : cases) {
        harness.addTest(group + ": " + c.name, [c]() { expectFormatted(c.input, c.expected); });
    }
}

} // namespace mtype::lsp::test::formatting
