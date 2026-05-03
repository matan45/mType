#include "SemanticTokensHandler.hpp"
#include <algorithm>
#include <sstream>
#include <unordered_set>

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

void SemanticTokensHandler::pushToken(std::vector<RawToken>& tokens,
                                       int line, int startChar, int length,
                                       int type, int modifiers) {
    if (length <= 0) return;
    tokens.push_back({line, startChar, length, type, modifiers});
}

// ---------- Constructor — pre-compile all regexes ----------

SemanticTokensHandler::SemanticTokensHandler(DocumentManager* docMgr)
    : annotationRegex_(R"(@(\w+))")
    , annotationDeclRegex_(R"(\bannotation\s+(\w+))")
    // `(value\s+|abstract\s+)?` lets the class-decl pass capture the
    // `class` keyword and the type name even when prefixed with the
    // value-class modifier (e.g. `public value class Int { ... }`).
    , classRegex_(R"(\b(value\s+|abstract\s+)?class\s+(\w+))")
    , interfaceRegex_(R"(\binterface\s+(\w+))")
    , methodRegex_(R"(\b(static\s+)?(async\s+)?function\s+(\w+)\s*\()")
    , varRegex_(R"(\b(?:public|private|protected)?\s*(?:static\s+)?(?:final\s+)?(\w+(?:<[^>]+>)?)\s+(\w+)\s*[;=])")
    // Single alternation — no per-keyword/per-modifier regex construction.
    // Keywords that are also modifiers are excluded here; they are handled
    // solely by tokenizeModifiers to avoid duplicate tokens at the same position.
    // `annotation` is excluded too — tokenizeAnnotationDeclarations handles
    // it so the type name following it can also be colored as a class.
    , keywordRegex_(R"(\b(if|else|while|do|for|switch|case|default|break|continue|match|return|new|this|super|try|catch|finally|throw|import|from|extends|implements|final|await|isClassOf)\b)")
    , modifierRegex_(R"(\b(public|private|protected|static|abstract|async|final|value)\b)")
    , functionCallRegex_(R"(\b(\w+)\s*\()")
    , classDeclLookbehind_(R"(\b(?:class|interface)\s*$)")
    , documentManager_(docMgr)
{
}

// ---------- Main entry point ----------

SemanticTokens SemanticTokensHandler::handleSemanticTokensFull(const std::string& uri) {
    // Thread-safe: use a local vector instead of a member scratch buffer
    std::vector<RawToken> tokens;

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
        tokenizeAnnotations(line, lineIndex, tokens);
        tokenizeAnnotationDeclarations(line, lineIndex, tokens);
        tokenizeClassDeclarations(line, lineIndex, tokens);
        tokenizeInterfaceDeclarations(line, lineIndex, tokens);
        tokenizeMethodDeclarations(line, lineIndex, tokens);
        tokenizeVariableDeclarations(line, lineIndex, tokens);
        tokenizeKeywords(line, lineIndex, tokens);
        tokenizeTypes(line, lineIndex, knownClasses, tokens);
        tokenizeModifiers(line, lineIndex, tokens);
        tokenizeFunctionCalls(line, lineIndex, tokens);
        lineIndex++;
    }

    // Sort tokens by (line, startChar)
    std::sort(tokens.begin(), tokens.end(), [](const RawToken& a, const RawToken& b) {
        if (a.line != b.line) return a.line < b.line;
        return a.startChar < b.startChar;
    });

    // Delta-encode
    SemanticTokens result;
    int prevLine = 0;
    int prevStart = 0;

    for (const auto& tok : tokens) {
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

void SemanticTokensHandler::tokenizeAnnotations(const std::string& line, int lineIndex,
                                                 std::vector<RawToken>& tokens) const {
    auto begin = std::sregex_iterator(line.begin(), line.end(), annotationRegex_);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        pushToken(tokens, lineIndex,
                  static_cast<int>(it->position()),
                  static_cast<int>((*it)[0].length()),
                  encodeTokenType("decorator"), 0);
    }
}

void SemanticTokensHandler::tokenizeClassDeclarations(const std::string& line, int lineIndex,
                                                       std::vector<RawToken>& tokens) const {
    auto begin = std::sregex_iterator(line.begin(), line.end(), classRegex_);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        // Group 1 captures `value ` / `abstract ` (with trailing space)
        // when present — the `class` keyword sits past it.
        bool hasModifier = (*it)[1].matched;
        int classKeywordOffset = static_cast<int>(it->position())
            + (hasModifier ? static_cast<int>((*it)[1].length()) : 0);

        // Highlight "class" keyword
        pushToken(tokens, lineIndex, classKeywordOffset, 5,
                  encodeTokenType("keyword"), 0);

        // Highlight class name — use the actual match position of group 2
        std::string className = (*it)[2].str();
        int nameOffset = static_cast<int>(it->position(2));
        pushToken(tokens, lineIndex, nameOffset, static_cast<int>(className.length()),
                  encodeTokenType("class"),
                  encodeTokenModifiers({"declaration"}));
    }
}

void SemanticTokensHandler::tokenizeAnnotationDeclarations(const std::string& line, int lineIndex,
                                                            std::vector<RawToken>& tokens) const {
    auto begin = std::sregex_iterator(line.begin(), line.end(), annotationDeclRegex_);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        int pos = static_cast<int>(it->position());

        // Highlight "annotation" keyword (length 10)
        pushToken(tokens, lineIndex, pos, 10,
                  encodeTokenType("keyword"), 0);

        // Highlight the declared annotation type name. We tag it as
        // `class` so themes that style class-name declarations also
        // style annotation-type declarations consistently — the LSP
        // semantic-tokens legend doesn't have a dedicated annotation
        // declaration kind.
        std::string name = (*it)[1].str();
        int namePos = static_cast<int>(it->position(1));
        pushToken(tokens, lineIndex, namePos, static_cast<int>(name.length()),
                  encodeTokenType("class"),
                  encodeTokenModifiers({"declaration"}));
    }
}

void SemanticTokensHandler::tokenizeInterfaceDeclarations(const std::string& line, int lineIndex,
                                                           std::vector<RawToken>& tokens) const {
    auto begin = std::sregex_iterator(line.begin(), line.end(), interfaceRegex_);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        int pos = static_cast<int>(it->position());

        // Highlight "interface" keyword
        pushToken(tokens, lineIndex, pos, 9,
                  encodeTokenType("keyword"), 0);

        // Highlight interface name — use the actual match position of group 1
        std::string name = (*it)[1].str();
        int namePos = static_cast<int>(it->position(1));
        pushToken(tokens, lineIndex, namePos, static_cast<int>(name.length()),
                  encodeTokenType("interface"),
                  encodeTokenModifiers({"declaration"}));
    }
}

void SemanticTokensHandler::tokenizeMethodDeclarations(const std::string& line, int lineIndex,
                                                        std::vector<RawToken>& tokens) const {
    auto begin = std::sregex_iterator(line.begin(), line.end(), methodRegex_);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        bool isStatic = (*it)[1].matched;
        bool isAsync = (*it)[2].matched;
        std::string methodName = (*it)[3].str();
        int offset = static_cast<int>(it->position());

        // Highlight "static" if present
        if (isStatic) {
            pushToken(tokens, lineIndex, offset, 6,
                      encodeTokenType("modifier"),
                      encodeTokenModifiers({"static"}));
            offset += static_cast<int>((*it)[1].length());
        }

        // Highlight "async" if present
        if (isAsync) {
            pushToken(tokens, lineIndex, offset, 5,
                      encodeTokenType("modifier"),
                      encodeTokenModifiers({"async"}));
            offset += static_cast<int>((*it)[2].length());
        }

        // Highlight "function" keyword
        pushToken(tokens, lineIndex, offset, 8,
                  encodeTokenType("keyword"), 0);

        // Highlight method name — use actual match position of group 3
        int namePos = static_cast<int>(it->position(3));
        std::vector<std::string> mods = {"declaration"};
        if (isStatic) mods.push_back("static");
        if (isAsync) mods.push_back("async");

        pushToken(tokens, lineIndex, namePos, static_cast<int>(methodName.length()),
                  encodeTokenType("method"),
                  encodeTokenModifiers(mods));
    }
}

void SemanticTokensHandler::tokenizeVariableDeclarations(const std::string& line, int lineIndex,
                                                          std::vector<RawToken>& tokens) const {
    auto begin = std::sregex_iterator(line.begin(), line.end(), varRegex_);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::string varName = (*it)[2].str();
        int namePos = static_cast<int>(it->position(2));

        std::string fullMatch = (*it)[0].str();
        bool isStatic = fullMatch.find("static") != std::string::npos;
        bool isFinal = fullMatch.find("final") != std::string::npos;

        std::vector<std::string> mods = {"declaration"};
        if (isStatic) mods.push_back("static");
        if (isFinal) mods.push_back("readonly");

        pushToken(tokens, lineIndex, namePos, static_cast<int>(varName.length()),
                  encodeTokenType("variable"),
                  encodeTokenModifiers(mods));
    }
}

void SemanticTokensHandler::tokenizeKeywords(const std::string& line, int lineIndex,
                                              std::vector<RawToken>& tokens) const {
    // Uses a single pre-compiled alternation regex.
    // Excludes words that are also modifiers (public, private, protected,
    // static, abstract, async) — those are handled by tokenizeModifiers
    // to avoid emitting duplicate tokens at the same position.
    auto begin = std::sregex_iterator(line.begin(), line.end(), keywordRegex_);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        pushToken(tokens, lineIndex,
                  static_cast<int>(it->position()),
                  static_cast<int>((*it)[0].length()),
                  encodeTokenType("keyword"), 0);
    }
}

void SemanticTokensHandler::tokenizeTypes(
    const std::string& line, int lineIndex,
    const std::vector<std::string>& knownClasses,
    std::vector<RawToken>& tokens) const
{
    // Build a single alternation regex for all known types.
    // This is per-request but only one regex per call (not per-type per-line).
    if (knownClasses.empty()) return;

    std::string pattern = "\\b(";
    for (size_t i = 0; i < knownClasses.size(); i++) {
        if (i > 0) pattern += "|";
        pattern += knownClasses[i];
    }
    pattern += ")\\b";

    std::regex typesRegex(pattern);
    auto begin = std::sregex_iterator(line.begin(), line.end(), typesRegex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        int pos = static_cast<int>(it->position());

        // Skip if this is part of a class/interface declaration (already handled)
        int lookback = std::max(0, pos - 20);
        std::string before = line.substr(lookback, pos - lookback);
        if (std::regex_search(before, classDeclLookbehind_)) {
            continue;
        }

        pushToken(tokens, lineIndex, pos, static_cast<int>((*it)[0].length()),
                  encodeTokenType("class"), 0);
    }
}

void SemanticTokensHandler::tokenizeModifiers(const std::string& line, int lineIndex,
                                               std::vector<RawToken>& tokens) const {
    // Uses a single pre-compiled alternation regex.
    // This is the sole emitter for modifier words (public, private, protected,
    // static, abstract, async, final). tokenizeKeywords excludes these to
    // prevent duplicate tokens at the same position.
    auto begin = std::sregex_iterator(line.begin(), line.end(), modifierRegex_);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::string mod = (*it)[1].str();

        std::vector<std::string> tokenMods;
        if (mod == "static") tokenMods.push_back("static");
        if (mod == "final") tokenMods.push_back("readonly");
        if (mod == "abstract") tokenMods.push_back("abstract");
        if (mod == "async") tokenMods.push_back("async");

        pushToken(tokens, lineIndex,
                  static_cast<int>(it->position()),
                  static_cast<int>(mod.length()),
                  encodeTokenType("modifier"),
                  encodeTokenModifiers(tokenMods));
    }
}

void SemanticTokensHandler::tokenizeFunctionCalls(const std::string& line, int lineIndex,
                                                    std::vector<RawToken>& tokens) const {
    auto begin = std::sregex_iterator(line.begin(), line.end(), functionCallRegex_);
    auto end = std::sregex_iterator();

    static const std::unordered_set<std::string> skipKeywords = {
        "if", "while", "for", "switch", "catch", "function"
    };

    for (auto it = begin; it != end; ++it) {
        std::string funcName = (*it)[1].str();
        if (skipKeywords.count(funcName)) continue;

        pushToken(tokens, lineIndex,
                  static_cast<int>(it->position()),
                  static_cast<int>(funcName.length()),
                  encodeTokenType("function"), 0);
    }
}

} // namespace mtype::lsp
