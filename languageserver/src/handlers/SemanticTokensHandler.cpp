#include "SemanticTokensHandler.hpp"
#include <algorithm>
#include <regex>
#include <sstream>

namespace mtype::lsp {

// ---------- Legend (must match the TS provider exactly) ----------

const std::vector<std::string>& SemanticTokensHandler::tokenTypes() {
    static const std::vector<std::string> types = {
        "namespace",      // 0
        "class",          // 1
        "enum",           // 2
        "interface",      // 3
        "struct",         // 4
        "typeParameter",  // 5
        "type",           // 6
        "parameter",      // 7
        "variable",       // 8
        "property",       // 9
        "function",       // 10
        "method",         // 11
        "keyword",        // 12
        "modifier",       // 13
        "comment",        // 14
        "string",         // 15
        "number",         // 16
        "regexp",         // 17
        "operator",       // 18
        "decorator"       // 19
    };
    return types;
}

const std::vector<std::string>& SemanticTokensHandler::tokenModifiers() {
    static const std::vector<std::string> mods = {
        "declaration",    // 0
        "definition",     // 1
        "readonly",       // 2
        "static",         // 3
        "deprecated",     // 4
        "abstract",       // 5
        "async",          // 6
        "modification",   // 7
        "documentation",  // 8
        "defaultLibrary"  // 9
    };
    return mods;
}

// ---------- Encoding helpers ----------

int SemanticTokensHandler::encodeTokenType(const std::string& type) {
    const auto& types = tokenTypes();
    for (size_t i = 0; i < types.size(); i++) {
        if (types[i] == type) return static_cast<int>(i);
    }
    return 0;
}

int SemanticTokensHandler::encodeTokenModifiers(const std::vector<std::string>& mods) {
    int result = 0;
    const auto& allMods = tokenModifiers();
    for (const auto& mod : mods) {
        for (size_t i = 0; i < allMods.size(); i++) {
            if (allMods[i] == mod) {
                result |= (1 << static_cast<int>(i));
                break;
            }
        }
    }
    return result;
}

void SemanticTokensHandler::pushToken(int line, int startChar, int length, int type, int modifiers) {
    if (length <= 0) return;
    tokens_.push_back({line, startChar, length, type, modifiers});
}

// ---------- Constructor ----------

SemanticTokensHandler::SemanticTokensHandler(DocumentManager* docMgr)
    : documentManager_(docMgr)
{
}

// ---------- Main entry point ----------

SemanticTokens SemanticTokensHandler::handleSemanticTokensFull(const std::string& uri) {
    tokens_.clear();

    auto* doc = documentManager_->getDocument(uri);
    if (!doc) return SemanticTokens{};

    // Collect known class names from document symbols
    std::vector<std::string> knownClasses;
    auto symbols = documentManager_->getDocumentSymbols(uri);
    for (const auto& sym : symbols) {
        if (sym.kind == "class" || sym.kind == "interface") {
            knownClasses.push_back(sym.name);
        }
    }

    // Standard collection types
    static const std::vector<std::string> standardTypes = {
        "List", "LinkedList", "HashMap", "HashSet", "Stack", "Queue",
        "Array", "Set", "Map", "Promise"
    };
    for (const auto& t : standardTypes) {
        knownClasses.push_back(t);
    }

    // Process each line
    std::istringstream stream(doc->content);
    std::string line;
    int lineIndex = 0;

    while (std::getline(stream, line)) {
        tokenizeAnnotations(line, lineIndex);
        tokenizeClassDeclarations(line, lineIndex);
        tokenizeInterfaceDeclarations(line, lineIndex);
        tokenizeMethodDeclarations(line, lineIndex);
        tokenizeVariableDeclarations(line, lineIndex);
        tokenizeKeywords(line, lineIndex);
        tokenizeTypes(line, lineIndex, knownClasses);
        tokenizeModifiers(line, lineIndex);
        tokenizeFunctionCalls(line, lineIndex);
        lineIndex++;
    }

    // Sort tokens by (line, startChar)
    std::sort(tokens_.begin(), tokens_.end(), [](const RawToken& a, const RawToken& b) {
        if (a.line != b.line) return a.line < b.line;
        return a.startChar < b.startChar;
    });

    // Delta-encode
    SemanticTokens result;
    int prevLine = 0;
    int prevStart = 0;

    for (const auto& tok : tokens_) {
        int deltaLine = tok.line - prevLine;
        int deltaStart = (deltaLine == 0) ? (tok.startChar - prevStart) : tok.startChar;

        result.data.push_back(static_cast<uint32_t>(deltaLine));
        result.data.push_back(static_cast<uint32_t>(deltaStart));
        result.data.push_back(static_cast<uint32_t>(tok.length));
        result.data.push_back(static_cast<uint32_t>(tok.tokenType));
        result.data.push_back(static_cast<uint32_t>(tok.tokenModifiers));

        prevLine = tok.line;
        prevStart = tok.startChar;
    }

    return result;
}

// ---------- Tokenization passes ----------

void SemanticTokensHandler::tokenizeAnnotations(const std::string& line, int lineIndex) {
    std::regex regex(R"(@(\w+))");
    auto begin = std::sregex_iterator(line.begin(), line.end(), regex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        pushToken(lineIndex,
                  static_cast<int>(it->position()),
                  static_cast<int>((*it)[0].length()),
                  encodeTokenType("decorator"),
                  0);
    }
}

void SemanticTokensHandler::tokenizeClassDeclarations(const std::string& line, int lineIndex) {
    std::regex regex(R"(\b(abstract\s+)?class\s+(\w+))");
    auto begin = std::sregex_iterator(line.begin(), line.end(), regex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        bool hasAbstract = (*it)[1].matched;
        int classKeywordOffset = static_cast<int>(it->position()) + (hasAbstract ? static_cast<int>((*it)[1].length()) : 0);

        // Highlight "class" keyword
        pushToken(lineIndex, classKeywordOffset, 5,
                  encodeTokenType("keyword"), 0);

        // Highlight class name
        std::string className = (*it)[2].str();
        int nameOffset = classKeywordOffset + 6; // "class " = 6 chars
        pushToken(lineIndex, nameOffset, static_cast<int>(className.length()),
                  encodeTokenType("class"),
                  encodeTokenModifiers({"declaration"}));
    }
}

void SemanticTokensHandler::tokenizeInterfaceDeclarations(const std::string& line, int lineIndex) {
    std::regex regex(R"(\binterface\s+(\w+))");
    auto begin = std::sregex_iterator(line.begin(), line.end(), regex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        int pos = static_cast<int>(it->position());

        // Highlight "interface" keyword
        pushToken(lineIndex, pos, 9,
                  encodeTokenType("keyword"), 0);

        // Highlight interface name
        std::string name = (*it)[1].str();
        pushToken(lineIndex, pos + 10, static_cast<int>(name.length()),
                  encodeTokenType("interface"),
                  encodeTokenModifiers({"declaration"}));
    }
}

void SemanticTokensHandler::tokenizeMethodDeclarations(const std::string& line, int lineIndex) {
    std::regex regex(R"(\b(static\s+)?(async\s+)?function\s+(\w+)\s*\()");
    auto begin = std::sregex_iterator(line.begin(), line.end(), regex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        bool isStatic = (*it)[1].matched;
        bool isAsync = (*it)[2].matched;
        std::string methodName = (*it)[3].str();
        int offset = static_cast<int>(it->position());

        // Highlight "static" if present
        if (isStatic) {
            pushToken(lineIndex, offset, 6,
                      encodeTokenType("modifier"),
                      encodeTokenModifiers({"static"}));
            offset += static_cast<int>((*it)[1].length());
        }

        // Highlight "async" if present
        if (isAsync) {
            pushToken(lineIndex, offset, 5,
                      encodeTokenType("modifier"),
                      encodeTokenModifiers({"async"}));
            offset += static_cast<int>((*it)[2].length());
        }

        // Highlight "function" keyword
        pushToken(lineIndex, offset, 8,
                  encodeTokenType("keyword"), 0);

        // Highlight method name
        int namePos = static_cast<int>(line.find(methodName, offset + 8));
        if (namePos >= 0) {
            std::vector<std::string> mods = {"declaration"};
            if (isStatic) mods.push_back("static");
            if (isAsync) mods.push_back("async");

            pushToken(lineIndex, namePos, static_cast<int>(methodName.length()),
                      encodeTokenType("method"),
                      encodeTokenModifiers(mods));
        }
    }
}

void SemanticTokensHandler::tokenizeVariableDeclarations(const std::string& line, int lineIndex) {
    std::regex regex(R"(\b(?:public|private|protected)?\s*(?:static\s+)?(?:final\s+)?(\w+(?:<[^>]+>)?)\s+(\w+)\s*[;=])");
    auto begin = std::sregex_iterator(line.begin(), line.end(), regex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::string varName = (*it)[2].str();
        int namePos = static_cast<int>(line.find(varName, static_cast<size_t>(it->position())));
        if (namePos < 0) continue;

        std::string fullMatch = (*it)[0].str();
        bool isStatic = fullMatch.find("static") != std::string::npos;
        bool isFinal = fullMatch.find("final") != std::string::npos;

        std::vector<std::string> mods = {"declaration"};
        if (isStatic) mods.push_back("static");
        if (isFinal) mods.push_back("readonly");

        pushToken(lineIndex, namePos, static_cast<int>(varName.length()),
                  encodeTokenType("variable"),
                  encodeTokenModifiers(mods));
    }
}

void SemanticTokensHandler::tokenizeKeywords(const std::string& line, int lineIndex) {
    static const std::vector<std::string> keywords = {
        "if", "else", "while", "do", "for", "switch", "case", "default", "break", "continue", "match",
        "return", "new", "this", "super", "try", "catch", "finally", "throw",
        "import", "from", "extends", "implements", "abstract", "final",
        "public", "private", "protected", "static", "async", "await"
    };

    for (const auto& kw : keywords) {
        std::regex regex("\\b" + kw + "\\b");
        auto begin = std::sregex_iterator(line.begin(), line.end(), regex);
        auto end = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            pushToken(lineIndex,
                      static_cast<int>(it->position()),
                      static_cast<int>(kw.length()),
                      encodeTokenType("keyword"), 0);
        }
    }
}

void SemanticTokensHandler::tokenizeTypes(
    const std::string& line, int lineIndex,
    const std::vector<std::string>& knownClasses)
{
    for (const auto& typeName : knownClasses) {
        std::regex regex("\\b" + typeName + "\\b");
        auto begin = std::sregex_iterator(line.begin(), line.end(), regex);
        auto end = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            int pos = static_cast<int>(it->position());

            // Skip if this is part of a class/interface declaration (already handled)
            int lookback = std::max(0, pos - 20);
            std::string before = line.substr(lookback, pos - lookback);
            if (std::regex_search(before, std::regex(R"(\b(?:class|interface)\s*$)"))) {
                continue;
            }

            pushToken(lineIndex, pos, static_cast<int>(typeName.length()),
                      encodeTokenType("class"), 0);
        }
    }
}

void SemanticTokensHandler::tokenizeModifiers(const std::string& line, int lineIndex) {
    static const std::vector<std::string> modifiers = {
        "public", "private", "protected", "static", "final", "abstract", "async"
    };

    for (const auto& mod : modifiers) {
        std::regex regex("\\b" + mod + "\\b");
        auto begin = std::sregex_iterator(line.begin(), line.end(), regex);
        auto end = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            std::vector<std::string> tokenMods;
            if (mod == "static") tokenMods.push_back("static");
            if (mod == "final") tokenMods.push_back("readonly");
            if (mod == "abstract") tokenMods.push_back("abstract");
            if (mod == "async") tokenMods.push_back("async");

            pushToken(lineIndex,
                      static_cast<int>(it->position()),
                      static_cast<int>(mod.length()),
                      encodeTokenType("modifier"),
                      encodeTokenModifiers(tokenMods));
        }
    }
}

void SemanticTokensHandler::tokenizeFunctionCalls(const std::string& line, int lineIndex) {
    std::regex regex(R"(\b(\w+)\s*\()");
    auto begin = std::sregex_iterator(line.begin(), line.end(), regex);
    auto end = std::sregex_iterator();

    static const std::vector<std::string> skipKeywords = {
        "if", "while", "for", "switch", "catch", "function"
    };

    for (auto it = begin; it != end; ++it) {
        std::string funcName = (*it)[1].str();

        bool isKeyword = false;
        for (const auto& kw : skipKeywords) {
            if (funcName == kw) { isKeyword = true; break; }
        }
        if (isKeyword) continue;

        pushToken(lineIndex,
                  static_cast<int>(it->position()),
                  static_cast<int>(funcName.length()),
                  encodeTokenType("function"), 0);
    }
}

} // namespace mtype::lsp
