#include "FormattingHandler.hpp"
#include "FormattingSpacingRules.hpp"
#include "../utils/SourceSegmenter.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <string_view>
#include <tuple>

namespace mtype::lsp {

namespace {

using utils::LineScan;
using utils::Region;
using utils::SourceSegmenter;

bool isBlankChar(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

bool isWordChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

bool isBlank(const std::string& text) {
    return std::all_of(text.begin(), text.end(), isBlankChar);
}

std::string trimLeft(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() && isBlankChar(text[start])) ++start;
    return text.substr(start);
}

std::string trimRight(std::string text) {
    std::size_t end = text.size();
    while (end > 0 && isBlankChar(text[end - 1])) --end;
    text.resize(end);
    return text;
}

// Split like std::getline: a final '\n' ends the last line instead of starting
// an empty one. One trailing '\r' per line is dropped, so CRLF becomes LF.
std::vector<std::string> splitLines(const std::string& content) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start < content.size()) {
        std::size_t newline = content.find('\n', start);
        std::size_t end = newline == std::string::npos ? content.size() : newline;
        std::string line = content.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(std::move(line));
        if (newline == std::string::npos) break;
        start = newline + 1;
    }
    return lines;
}

std::string joinLines(const std::vector<std::string>& lines, bool finalNewline) {
    std::string text;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) text += '\n';
        text += lines[i];
    }
    if (finalNewline) text += '\n';
    return text;
}

std::string createIndent(int level, const FormattingOptions& options) {
    if (options.insertSpaces) {
        return std::string(level * options.tabSize, ' ');
    }
    return std::string(level, '\t');
}

std::string formatLine(const std::string& line, const LineScan& scan, int indentLevel,
                       const FormattingOptions& options) {
    if (scan.startsIn == Region::Code && isBlank(line)) {
        return "";
    }
    std::string text = FormattingSpacingRules::apply(line, scan);
    // A line that starts inside a string literal or block comment keeps its
    // leading whitespace: in a string it is part of the value, and comment
    // bodies are the author's text.
    if (scan.startsIn == Region::Code) {
        text = createIndent(indentLevel, options) + trimLeft(text);
    }
    // Trailing spaces before a line break inside a string are part of it too.
    if (options.trimTrailingWhitespace && scan.endsIn != Region::String) {
        text = trimRight(std::move(text));
    }
    return text;
}

// A one-line `import ... ;` statement: whole-word keyword, entirely code, and
// the `;` is its last code character. A multi-line `import {` is therefore
// left in place instead of being separated from its continuation lines.
bool isImportLine(const std::string& line, const LineScan& scan) {
    if (scan.startsIn != Region::Code || scan.endsIn != Region::Code) {
        return false;
    }
    constexpr std::string_view keyword = "import";
    const std::string text = trimLeft(line);
    if (text.compare(0, keyword.size(), keyword) != 0) {
        return false;
    }
    if (text.size() > keyword.size() && isWordChar(text[keyword.size()])) {
        return false;
    }
    return utils::lastCodeChar(line, scan) == ';';
}

std::string extractImportPath(const std::string& importLine) {
    static const std::regex pathRegex(R"(from\s+["']([^"']+)["'])");
    std::smatch match;
    return std::regex_search(importLine, match, pathRegex) ? match[1].str() : std::string();
}

int countRelativeDepth(const std::string& path) {
    int depth = 0;
    for (std::size_t pos = path.find("../"); pos != std::string::npos; pos = path.find("../", pos + 3)) {
        ++depth;
    }
    return depth;
}

// Shallower relative paths first, then by path, then by the full line, so the
// order is total and a second format cannot reorder equal paths.
void sortAndDedupImports(std::vector<std::string>& imports) {
    auto key = [](const std::string& line) {
        std::string path = extractImportPath(line);
        int depth = countRelativeDepth(path);
        return std::make_tuple(depth, std::move(path), line);
    };
    std::sort(imports.begin(), imports.end(),
        [&](const std::string& a, const std::string& b) { return key(a) < key(b); });
    imports.erase(std::unique(imports.begin(), imports.end()), imports.end());
}

} // namespace

std::vector<TextEdit> FormattingHandler::formatDocument(
    const std::string& content,
    const FormattingOptions& options
) {
    const std::vector<std::string> originalLines = splitLines(content);
    const FormattedLines formatted = formatLines(organizeImports(originalLines), options);

    // Keep a final newline the document already had. Add one only when asked,
    // and never at the end of an unterminated string or comment, where it
    // would become part of that literal's text.
    const bool hadFinalNewline = !content.empty() && content.back() == '\n';
    const bool finalNewline = !formatted.lines.empty()
        && (hadFinalNewline || (options.insertFinalNewline && !formatted.endsInsideStringOrComment));

    // One edit replacing the whole ORIGINAL document. Its end must come from
    // the original line count: organizeImports can add or remove lines, and a
    // short range would leave the original tail behind, duplicating code.
    TextEdit edit;
    edit.range.start.line = 0;
    edit.range.start.character = 0;
    edit.range.end.line = static_cast<int>(originalLines.size());
    edit.range.end.character = 0;
    edit.newText = joinLines(formatted.lines, finalNewline);
    return {edit};
}

std::vector<TextEdit> FormattingHandler::formatRange(
    const std::string& content,
    const Range& range,
    const FormattingOptions& options
) {
    // For now, format the entire document
    // TODO: Implement range-specific formatting
    return formatDocument(content, options);
}

FormattingHandler::FormattedLines FormattingHandler::formatLines(
    const std::vector<std::string>& lines,
    const FormattingOptions& options
) const {
    FormattedLines result;
    result.lines.reserve(lines.size());
    SourceSegmenter segmenter;
    int indent = 0;

    for (const auto& line : lines) {
        const LineScan scan = segmenter.scanLine(line);
        // Only braces that are code count, never ones inside strings or comments.
        if (utils::firstCodeChar(line, scan) == '}') {
            indent = std::max(0, indent - 1);
        }
        result.lines.push_back(formatLine(line, scan, indent, options));
        if (utils::lastCodeChar(line, scan) == '{') {
            ++indent;
        }
    }

    result.endsInsideStringOrComment = segmenter.region() != Region::Code;
    return result;
}

std::vector<std::string> FormattingHandler::organizeImports(const std::vector<std::string>& lines) const {
    SourceSegmenter segmenter;
    std::vector<std::string> imports;
    std::size_t next = 0;

    // Blank lines inside the import block are dropped; the block ends at the
    // first line that is not a one-line import.
    for (; next < lines.size(); ++next) {
        const LineScan scan = segmenter.scanLine(lines[next]);
        if (isBlank(lines[next])) continue;
        if (!isImportLine(lines[next], scan)) break;
        // Normalize spacing first so de-duplication sees the final text.
        imports.push_back(trimRight(trimLeft(FormattingSpacingRules::apply(lines[next], scan))));
    }

    if (imports.empty()) {
        return lines;
    }

    sortAndDedupImports(imports);
    if (next < lines.size()) {
        imports.emplace_back();  // exactly one blank line between imports and code
        imports.insert(imports.end(), lines.begin() + static_cast<std::ptrdiff_t>(next), lines.end());
    }
    return imports;
}

} // namespace mtype::lsp
