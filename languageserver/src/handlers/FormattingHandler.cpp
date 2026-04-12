#include "FormattingHandler.hpp"
#include <sstream>
#include <algorithm>
#include <regex>

namespace mtype::lsp {

std::vector<TextEdit> FormattingHandler::formatDocument(
    const std::string& content,
    const FormattingOptions& options
) {
    std::vector<TextEdit> edits;

    // Pre-process: organize imports before formatting
    std::string organized = organizeImports(content);

    std::istringstream stream(organized);
    std::string line;
    std::ostringstream formattedContent;
    int currentIndent = 0;
    int lineNumber = 0;

    while (std::getline(stream, line)) {
        // Skip empty lines (just preserve them)
        if (trim(line).empty()) {
            formattedContent << "\n";
            lineNumber++;
            continue;
        }

        // Check if this line should decrease indent first (closing brace)
        if (shouldDecreaseIndent(trim(line))) {
            currentIndent = std::max(0, currentIndent - 1);
        }

        // Format the line with current indent
        std::string formatted = formatLine(line, currentIndent, options);
        formattedContent << formatted << "\n";

        // Check if next line should increase indent (opening brace)
        if (shouldIncreaseIndent(trim(line))) {
            currentIndent++;
        }

        lineNumber++;
    }

    // Create a single edit that replaces entire document
    TextEdit edit;
    edit.range.start.line = 0;
    edit.range.start.character = 0;

    // Calculate end position (last line, last character)
    std::istringstream countStream(organized);
    int totalLines = 0;
    std::string lastLine;
    while (std::getline(countStream, lastLine)) {
        totalLines++;
    }

    edit.range.end.line = totalLines;
    edit.range.end.character = 0;

    std::string result = formattedContent.str();

    // Remove trailing newline if content didn't have one
    if (!organized.empty() && organized.back() != '\n' && !result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    // Trim trailing whitespace from each line if enabled
    if (options.trimTrailingWhitespace) {
        std::istringstream trimStream(result);
        std::ostringstream trimmed;
        std::string trimLine;
        while (std::getline(trimStream, trimLine)) {
            // Remove trailing spaces
            size_t endPos = trimLine.find_last_not_of(" \t\r");
            if (endPos != std::string::npos) {
                trimmed << trimLine.substr(0, endPos + 1) << "\n";
            } else {
                trimmed << "\n";
            }
        }
        result = trimmed.str();

        // Remove trailing newline again if needed
        if (!organized.empty() && organized.back() != '\n' && !result.empty() && result.back() == '\n') {
            result.pop_back();
        }
    }

    // Add final newline if enabled and missing
    if (options.insertFinalNewline && !result.empty() && result.back() != '\n') {
        result += '\n';
    }

    edit.newText = result;
    edits.push_back(edit);

    return edits;
}

std::string FormattingHandler::organizeImports(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    std::vector<std::string> imports;
    std::vector<std::string> nonImports;
    bool inImportSection = true;

    while (std::getline(stream, line)) {
        std::string trimmed = trim(line);
        if (inImportSection && trimmed.substr(0, 7) == "import ") {
            imports.push_back(trimmed);
        } else {
            // The first non-empty, non-import line ends the import section.
            // Blank lines between imports are tolerated.
            if (!trimmed.empty()) {
                inImportSection = false;
            }
            nonImports.push_back(line);
        }
    }

    if (imports.empty()) {
        return content;
    }

    // Extract the path from an import line for sorting
    auto extractPath = [](const std::string& importLine) -> std::string {
        // Match: from "path" or from 'path'
        std::regex pathRegex(R"(from\s+["']([^"']+)["'])");
        std::smatch match;
        if (std::regex_search(importLine, match, pathRegex)) {
            return match[1].str();
        }
        return "";
    };

    // Count relative depth (number of ../ segments)
    auto countDepth = [](const std::string& path) -> int {
        int depth = 0;
        size_t pos = 0;
        while ((pos = path.find("../", pos)) != std::string::npos) {
            depth++;
            pos += 3;
        }
        return depth;
    };

    // Sort imports by: (1) relative depth ascending, (2) path alphabetically
    std::sort(imports.begin(), imports.end(),
        [&](const std::string& a, const std::string& b) {
            std::string pathA = extractPath(a);
            std::string pathB = extractPath(b);
            int depthA = countDepth(pathA);
            int depthB = countDepth(pathB);
            if (depthA != depthB) return depthA < depthB;
            return pathA < pathB;
        });

    // Deduplicate
    imports.erase(std::unique(imports.begin(), imports.end()), imports.end());

    // Reassemble: sorted imports, blank separator, then rest
    std::ostringstream result;
    for (const auto& imp : imports) {
        result << imp << "\n";
    }

    // Add blank line between imports and code if there is non-empty code
    bool hasNonEmptyCode = false;
    for (const auto& l : nonImports) {
        if (!trim(l).empty()) { hasNonEmptyCode = true; break; }
    }
    if (hasNonEmptyCode) {
        result << "\n";
    }

    for (size_t i = 0; i < nonImports.size(); i++) {
        result << nonImports[i];
        if (i + 1 < nonImports.size()) {
            result << "\n";
        }
    }

    return result.str();
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

std::string FormattingHandler::formatLine(
    const std::string& line,
    int indentLevel,
    const FormattingOptions& options
) {
    std::string trimmed = trim(line);

    if (trimmed.empty()) {
        return "";
    }

    // Create indentation
    std::string indent = createIndent(indentLevel, options);

    // Format operators (add spaces around =, +, -, etc.)
    trimmed = formatOperators(trimmed);

    // Format declarations
    trimmed = formatDeclaration(trimmed);

    return indent + trimmed;
}

int FormattingHandler::calculateIndentLevel(const std::string& line, int currentIndent) {
    std::string trimmed = trim(line);

    if (shouldDecreaseIndent(trimmed)) {
        return std::max(0, currentIndent - 1);
    }

    return currentIndent;
}

bool FormattingHandler::shouldDecreaseIndent(const std::string& line) {
    std::string trimmed = trim(line);

    // Lines starting with closing braces
    if (!trimmed.empty() && trimmed[0] == '}') {
        return true;
    }

    return false;
}

bool FormattingHandler::shouldIncreaseIndent(const std::string& line) {
    std::string trimmed = trim(line);

    // Lines ending with opening braces
    if (!trimmed.empty() && trimmed.back() == '{') {
        return true;
    }

    return false;
}

std::string FormattingHandler::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }

    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string FormattingHandler::formatOperators(const std::string& line) {
    std::string result = line;

    // Add spaces around binary operators: =, +, -, *, /, ==, !=, <=, >=, <, >
    // But not in strings or after colons

    // Simple implementation: add space before and after = if not already present
    std::regex assignRegex(R"((\w)=(\w))");
    result = std::regex_replace(result, assignRegex, "$1 = $2");

    // Add space after commas
    std::regex commaRegex(R"(,(\S))");
    result = std::regex_replace(result, commaRegex, ", $1");

    // Add space after colons (in type annotations)
    std::regex colonRegex(R"(:(\w))");
    result = std::regex_replace(result, colonRegex, ": $1");

    return result;
}

std::string FormattingHandler::formatDeclaration(const std::string& line) {
    std::string result = line;

    // Ensure space after keywords: class, interface, function, etc.
    std::vector<std::string> keywords = {
        "class", "interface", "function", "if", "else", "while",
        "for", "foreach", "return", "new", "import", "from",
        "public", "private", "protected", "static", "final",
        "abstract", "async", "await"
    };

    for (const auto& keyword : keywords) {
        std::regex keywordRegex("\\b" + keyword + "(\\S)");
        result = std::regex_replace(result, keywordRegex, keyword + " $1");
    }

    return result;
}

std::string FormattingHandler::createIndent(int level, const FormattingOptions& options) {
    if (options.insertSpaces) {
        return std::string(level * options.tabSize, ' ');
    } else {
        return std::string(level, '\t');
    }
}

} // namespace mtype::lsp
