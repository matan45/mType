#include "CodeLensHandler.hpp"
#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/InterfaceNode.hpp"
#include "../../../mType/ast/nodes/statements/ProgramNode.hpp"
#include <algorithm>
#include <regex>
#include <sstream>

namespace mtype::lsp {

namespace {

struct LocalLensSymbol {
    std::string name;
    int line;
    int column;
};

int toLspLine(const errors::SourceLocation& loc) {
    return std::max(0, loc.getLine() - 1);
}

int toLspColumn(const errors::SourceLocation& loc) {
    return std::max(0, loc.getColumn() - 1);
}

std::string lineAt(const std::string& content, int targetLine) {
    std::istringstream stream(content);
    std::string line;
    int lineNum = 0;
    while (std::getline(stream, line)) {
        if (lineNum == targetLine) {
            return line;
        }
        lineNum++;
    }
    return "";
}

int findNameColumn(const std::string& content,
                   const std::string& name,
                   int line,
                   int fallbackColumn) {
    std::string text = lineAt(content, line);
    if (text.empty()) {
        return fallbackColumn;
    }

    auto start = static_cast<std::size_t>(std::max(0, fallbackColumn));
    auto pos = text.find(name, start);
    if (pos == std::string::npos) {
        pos = text.find(name);
    }
    return pos == std::string::npos ? fallbackColumn : static_cast<int>(pos);
}

std::vector<LocalLensSymbol> collectLocalLensSymbols(const Document* doc) {
    std::vector<LocalLensSymbol> symbols;
    if (!doc || !doc->isParsed || doc->ast.empty()) {
        return symbols;
    }

    auto* program = dynamic_cast<ast::nodes::statements::ProgramNode*>(doc->ast[0].get());
    if (!program) {
        return symbols;
    }

    for (const auto& statement : program->getStatements()) {
        ast::ASTNode* node = statement.get();
        if (!node) {
            continue;
        }

        if (auto* cls = dynamic_cast<ast::nodes::classes::ClassNode*>(node)) {
            const int line = toLspLine(cls->getLocation());
            const int fallbackColumn = toLspColumn(cls->getLocation());
            symbols.push_back(LocalLensSymbol{
                cls->getClassName(),
                line,
                findNameColumn(doc->content, cls->getClassName(), line, fallbackColumn)
            });
        } else if (auto* iface = dynamic_cast<ast::nodes::classes::InterfaceNode*>(node)) {
            const int line = toLspLine(iface->getLocation());
            const int fallbackColumn = toLspColumn(iface->getLocation());
            symbols.push_back(LocalLensSymbol{
                iface->getName(),
                line,
                findNameColumn(doc->content, iface->getName(), line, fallbackColumn)
            });
        }
    }

    return symbols;
}

std::string maskCommentsAndStrings(const std::string& content) {
    std::string masked = content;
    enum class State {
        Code,
        LineComment,
        BlockComment,
        String
    };

    State state = State::Code;
    char quote = '\0';

    for (std::size_t i = 0; i < masked.size(); ++i) {
        char c = masked[i];

        switch (state) {
            case State::Code:
                if (c == '/' && i + 1 < masked.size() && masked[i + 1] == '/') {
                    masked[i] = ' ';
                    masked[i + 1] = ' ';
                    ++i;
                    state = State::LineComment;
                } else if (c == '/' && i + 1 < masked.size() && masked[i + 1] == '*') {
                    masked[i] = ' ';
                    masked[i + 1] = ' ';
                    ++i;
                    state = State::BlockComment;
                } else if (c == '"' || c == '\'') {
                    quote = c;
                    masked[i] = ' ';
                    state = State::String;
                }
                break;

            case State::LineComment:
                if (c == '\n' || c == '\r') {
                    state = State::Code;
                } else {
                    masked[i] = ' ';
                }
                break;

            case State::BlockComment:
                if (c == '*' && i + 1 < masked.size() && masked[i + 1] == '/') {
                    masked[i] = ' ';
                    masked[i + 1] = ' ';
                    ++i;
                    state = State::Code;
                } else if (c != '\n' && c != '\r') {
                    masked[i] = ' ';
                }
                break;

            case State::String:
                if (c == '\\' && i + 1 < masked.size()) {
                    masked[i] = ' ';
                    if (masked[i + 1] != '\n' && masked[i + 1] != '\r') {
                        masked[i + 1] = ' ';
                    }
                    ++i;
                } else if (c == quote) {
                    masked[i] = ' ';
                    state = State::Code;
                } else if (c != '\n' && c != '\r') {
                    masked[i] = ' ';
                }
                break;
        }
    }

    return masked;
}

} // namespace

// Helper function to URL decode a string
static std::string urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            // Convert hex to char
            int value;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }

    return result;
}

CodeLensHandler::CodeLensHandler(DocumentManager* docMgr)
    : documentManager_(docMgr) {}

std::vector<CodeLens> CodeLensHandler::handleCodeLens(const std::string& uri) {
    std::vector<CodeLens> lenses;

    auto doc = documentManager_->getDocument(uri);
    if (!doc) {
        return lenses;
    }

    for (const auto& symbol : collectLocalLensSymbols(doc)) {
        int refCount = countReferences(symbol.name, doc, symbol.line);
        if (refCount == 0) {
            continue;
        }

        CodeLens lens;
        // Place range at start of line (character 0) so it renders above the declaration
        lens.range.start.line = symbol.line;
        lens.range.start.character = 0;
        lens.range.end.line = symbol.line;
        lens.range.end.character = 0;

        // Create command to show references using custom mtype.showReferences command
        Command cmd;
        if (refCount == 1) {
            cmd.title = "1 reference";
        } else {
            cmd.title = std::to_string(refCount) + " references";
        }
        cmd.command = "mtype.showReferences"; // Use custom command that converts JSON to VS Code types

        // Find actual reference locations
        std::vector<Location> refLocations = findUsages(symbol.name, doc, symbol.line);
        // Decode URI for VS Code command
        std::string decodedUri = urlDecode(uri);

        // Build arguments: uri, position, locations
        json args = json::array();
        args.push_back(decodedUri);

        json positionJson = {
            {"line", symbol.line},
            {"character", symbol.column}
        };
        args.push_back(positionJson);

        json locationsArray = json::array();
        for (const auto& loc : refLocations) {
            locationsArray.push_back(loc.toJson());
        }
        args.push_back(locationsArray);

        cmd.arguments = args;

        lens.command = cmd;
        lenses.push_back(lens);
    }

    return lenses;
}

int CodeLensHandler::countReferences(const std::string& symbolName, const Document* doc, int declarationLine) {
    if (!doc) {
        return 0;
    }

    return static_cast<int>(findUsages(symbolName, doc, declarationLine).size());
}

std::vector<Location> CodeLensHandler::findUsages(const std::string& symbolName, const Document* doc, int declarationLine) {
    std::vector<Location> locations;

    if (!doc) {
        return locations;
    }

    const std::string content = maskCommentsAndStrings(doc->content);

    // Use regex to find all references to the symbol
    std::string pattern = "\\b" + symbolName + "\\b";
    std::regex symbolRegex(pattern);

    std::string line;
    std::istringstream stream(content);
    int lineNum = 0;

    while (std::getline(stream, line)) {
        // Find all matches in this line
        auto words_begin = std::sregex_iterator(line.begin(), line.end(), symbolRegex);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            size_t matchPos = match.position();

            if (lineNum == declarationLine) {
                continue;
            }

            // Create a location for this reference
            Location loc;
            loc.uri = urlDecode(doc->uri); // Decode URI for VS Code
            loc.range.start.line = lineNum;
            loc.range.start.character = static_cast<int>(matchPos);
            loc.range.end.line = lineNum;
            loc.range.end.character = static_cast<int>(matchPos + symbolName.length());

            locations.push_back(loc);
        }

        lineNum++;
    }

    return locations;
}

} // namespace mtype::lsp
