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
    // Format a single line
    std::string formatLine(
        const std::string& line,
        int indentLevel,
        const FormattingOptions& options
    );

    // Calculate indent level based on braces
    int calculateIndentLevel(const std::string& line, int currentIndent);

    // Check if line should decrease indent (closing brace)
    bool shouldDecreaseIndent(const std::string& line);

    // Check if line should increase indent for next line (opening brace)
    bool shouldIncreaseIndent(const std::string& line);

    // Organize import statements (sort by depth, deduplicate)
    std::string organizeImports(const std::string& content);

    // Trim whitespace from line
    std::string trim(const std::string& str);

    // Add proper spacing around operators
    std::string formatOperators(const std::string& line);

    // Format function/class declarations
    std::string formatDeclaration(const std::string& line);

    // Create indentation string
    std::string createIndent(int level, const FormattingOptions& options);
};

} // namespace mtype::lsp
