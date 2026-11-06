#pragma once

#include <vector>
#include <string>
#include "../utils/LSPTypes.hpp"

namespace mtype::lsp {

struct FormattingOptions {
    int tabSize = 4;
    bool insertSpaces = true;  // Use spaces instead of tabs
    bool trimTrailingWhitespace = true;
    bool insertFinalNewline = true;
};

/**
 * Document formatter. It only changes whitespace between code tokens: string
 * literals and comments are never rewritten, lines that start inside a
 * multi-line string or block comment are kept as written, and formatting a
 * second time changes nothing (MYT-443).
 */
class FormattingHandler {
public:
    FormattingHandler() = default;

    // Format entire document
    std::vector<TextEdit> formatDocument(
        const std::string& content,
        const FormattingOptions& options
    );

    // Format a range of lines
    std::vector<TextEdit> formatRange(
        const std::string& content,
        const Range& range,
        const FormattingOptions& options
    );

private:
    struct FormattedLines {
        std::vector<std::string> lines;
        bool endsInsideStringOrComment = false;
    };

    // Re-indent and space every line. Produces exactly one output line per
    // input line, so range formatting (MYT-298) can take a slice of it.
    FormattedLines formatLines(const std::vector<std::string>& lines,
                               const FormattingOptions& options) const;

    // Sort and de-duplicate the leading block of one-line import statements,
    // followed by exactly one blank line.
    std::vector<std::string> organizeImports(const std::vector<std::string>& lines) const;
};

} // namespace mtype::lsp
