#include "SemanticTokensHandler.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>
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
    , parameterRegex_(R"mt(\b(?:final\s+)?([A-Za-z_]\w*(?:\s*<[^,(){}]*>)?(?:\s*\[\])?[?]?)\s+([A-Za-z_]\w*)\b(?=\s*[,)]))mt")
    , varRegex_(R"(\b(?:public|private|protected)?\s*(?:static\s+)?(?:final\s+)?(\w+(?:<[^>]+>)?(?:\s*\[\])?[?]?)\s+(\w+)\s*[;=])")
    , memberAccessRegex_(R"((\.|::)\s*([A-Za-z_]\w*))")
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
    auto* doc = documentManager_->getDocument(uri);
    if (!doc) return SemanticTokens{};

    std::vector<std::string> knownClasses = collectKnownClasses(uri);
    std::vector<RawToken> tokens;
    TokenizationState state;

    std::istringstream stream(doc->content);
    std::string line;
    int lineIndex = 0;
    while (std::getline(stream, line)) {
        processSourceLine(line, lineIndex, knownClasses, state, tokens);
        lineIndex++;
    }

    return deltaEncodeTokens(tokens);
}

std::vector<std::string> SemanticTokensHandler::collectKnownClasses(const std::string& uri) const {
    std::vector<std::string> knownClasses;
    auto symbols = documentManager_->getDocumentSymbols(uri);
    for (const auto& sym : symbols) {
        if (sym.kind == "class" || sym.kind == "interface") {
            knownClasses.push_back(sym.name);
        }
    }
    static const std::vector<std::string> standardTypes = {
        "List", "LinkedList", "HashMap", "HashSet", "Stack", "Queue",
        "Array", "Set", "Map", "Promise"
    };
    for (const auto& t : standardTypes) {
        knownClasses.push_back(t);
    }
    return knownClasses;
}

void SemanticTokensHandler::processSourceLine(const std::string& rawLine, int lineIndex,
                                              const std::vector<std::string>& knownClasses,
                                              TokenizationState& state,
                                              std::vector<RawToken>& tokens) {
    size_t lineCommentStart = findLineCommentStart(rawLine);
    std::string codeLine = lineCommentStart == std::string::npos
        ? rawLine
        : rawLine.substr(0, lineCommentStart);
    std::string semanticLine = maskStringLiterals(codeLine, lineIndex, tokens);
    bool classMemberContext = state.classDepth > 0 && state.functionDepth == 0 && !state.pendingFunctionBody;
    if (std::regex_search(semanticLine, methodRegex_)) {
        state.localSymbols.clear();
        state.parameterSymbols.clear();
    }

    tokenizeAnnotations(semanticLine, lineIndex, tokens);
    tokenizeAnnotationDeclarations(semanticLine, lineIndex, tokens);
    tokenizeClassDeclarations(semanticLine, lineIndex, tokens);
    tokenizeInterfaceDeclarations(semanticLine, lineIndex, tokens);
    tokenizeMethodDeclarations(semanticLine, lineIndex, tokens);
    tokenizeParameters(semanticLine, lineIndex, state.parameterSymbols, tokens);
    tokenizeVariableDeclarations(semanticLine, lineIndex, classMemberContext, state.localSymbols, tokens);
    tokenizeKeywords(semanticLine, lineIndex, tokens);
    tokenizeTypes(semanticLine, lineIndex, knownClasses, tokens);
    tokenizeMemberAccess(semanticLine, lineIndex, tokens);
    tokenizeModifiers(semanticLine, lineIndex, tokens);
    tokenizeFunctionCalls(semanticLine, lineIndex, tokens);
    tokenizeSymbolUsages(semanticLine, lineIndex, state.parameterSymbols, state.localSymbols, tokens);
    if (lineCommentStart != std::string::npos) {
        tokenizeLineComment(rawLine, lineIndex, lineCommentStart, tokens);
    }

    if (std::regex_search(semanticLine, classRegex_)
        || std::regex_search(semanticLine, interfaceRegex_)
        || std::regex_search(semanticLine, annotationDeclRegex_)) {
        state.pendingClassBody = true;
    }
    if (std::regex_search(semanticLine, methodRegex_)) {
        state.pendingFunctionBody = true;
    }

    updateScopeDepth(semanticLine, state);
}

void SemanticTokensHandler::updateScopeDepth(const std::string& semanticLine,
                                             TokenizationState& state) {
    int opens = countBraces(semanticLine, '{');
    int closes = countBraces(semanticLine, '}');
    for (int i = 0; i < opens; i++) {
        if (state.pendingFunctionBody) {
            state.functionDepth++;
            state.pendingFunctionBody = false;
        } else if (state.pendingClassBody) {
            state.classDepth++;
            state.pendingClassBody = false;
        } else if (state.functionDepth > 0) {
            state.functionDepth++;
        } else if (state.classDepth > 0) {
            state.classDepth++;
        }
    }
    for (int i = 0; i < closes; i++) {
        if (state.functionDepth > 0) {
            state.functionDepth--;
            if (state.functionDepth == 0) {
                state.localSymbols.clear();
                state.parameterSymbols.clear();
            }
        } else if (state.classDepth > 0) {
            state.classDepth--;
        }
    }
}

SemanticTokens SemanticTokensHandler::deltaEncodeTokens(std::vector<RawToken>& tokens) {
    std::sort(tokens.begin(), tokens.end(), [](const RawToken& a, const RawToken& b) {
        if (a.line != b.line) return a.line < b.line;
        return a.startChar < b.startChar;
    });

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

int SemanticTokensHandler::countBraces(const std::string& sourceLine, char brace) {
    std::string line = sourceLine;
    size_t comment = line.find("//");
    if (comment != std::string::npos) {
        line = line.substr(0, comment);
    }
    int count = 0;
    bool inString = false;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (c == '"' && (i == 0 || line[i - 1] != '\\')) {
            inString = !inString;
        }
        if (!inString && c == brace) {
            count++;
        }
    }
    return count;
}

size_t SemanticTokensHandler::findLineCommentStart(const std::string& sourceLine) {
    bool inString = false;
    char stringDelimiter = '\0';
    for (size_t i = 0; i + 1 < sourceLine.size(); i++) {
        char c = sourceLine[i];
        if ((c == '"' || c == '\'') && (i == 0 || sourceLine[i - 1] != '\\')) {
            if (inString && c == stringDelimiter) {
                inString = false;
                stringDelimiter = '\0';
            } else if (!inString) {
                inString = true;
                stringDelimiter = c;
            }
        }
        if (!inString && c == '/' && sourceLine[i + 1] == '/') {
            return i;
        }
    }
    return std::string::npos;
}

std::string SemanticTokensHandler::maskStringLiterals(const std::string& sourceLine,
                                                     int currentLine,
                                                     std::vector<RawToken>& rawTokens) {
    std::string masked = sourceLine;
    size_t i = 0;
    while (i < sourceLine.size()) {
        char c = sourceLine[i];
        if (c != '"' && c != '\'') {
            ++i;
            continue;
        }
        char quote = c;
        size_t start = i;
        size_t end = i + 1;
        while (end < sourceLine.size()) {
            char ec = sourceLine[end];
            // Forward scan: an escape always consumes the next char, so
            // `"\\"` resolves correctly — the closing `"` isn't masked as
            // still-escaped (the prior look-behind variant had this bug).
            if (ec == '\\' && end + 1 < sourceLine.size()) {
                end += 2;
                continue;
            }
            if (ec == quote) {
                ++end;
                break;
            }
            ++end;
        }
        pushToken(rawTokens, currentLine,
                  static_cast<int>(start),
                  static_cast<int>(end - start),
                  encodeTokenType("string"), 0);
        for (size_t j = start; j < end && j < masked.size(); ++j) {
            masked[j] = ' ';
        }
        i = end;
    }
    return masked;
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

void SemanticTokensHandler::tokenizeLineComment(const std::string& line, int lineIndex,
                                                 size_t commentStart,
                                                 std::vector<RawToken>& tokens) const {
    if (commentStart >= line.size()) return;

    pushToken(tokens, lineIndex,
              static_cast<int>(commentStart),
              static_cast<int>(line.size() - commentStart),
              encodeTokenType("comment"), 0);
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

void SemanticTokensHandler::tokenizeParameters(const std::string& line, int lineIndex,
                                                std::unordered_set<std::string>& parameterSymbols,
                                                std::vector<RawToken>& tokens) const {
    auto begin = std::sregex_iterator(line.begin(), line.end(), parameterRegex_);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::string typeName = (*it)[1].str();
        std::string paramName = (*it)[2].str();

        static const std::unordered_set<std::string> skipTypes = {
            "if", "while", "for", "switch", "catch", "return", "new", "function",
            "class", "interface", "annotation", "public", "private", "protected",
            "static", "final", "abstract", "async"
        };
        if (skipTypes.count(typeName)) continue;
        parameterSymbols.insert(paramName);

        pushToken(tokens, lineIndex,
                  static_cast<int>(it->position(2)),
                  static_cast<int>(paramName.length()),
                  encodeTokenType("parameter"),
                  encodeTokenModifiers({"declaration"}));
    }
}

void SemanticTokensHandler::tokenizeVariableDeclarations(const std::string& line, int lineIndex,
                                                          bool classMemberContext,
                                                          std::unordered_set<std::string>& localSymbols,
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
        if (!classMemberContext) {
            localSymbols.insert(varName);
        }

        pushToken(tokens, lineIndex, namePos, static_cast<int>(varName.length()),
                  encodeTokenType(classMemberContext ? "property" : "variable"),
                  encodeTokenModifiers(mods));
    }
}

void SemanticTokensHandler::tokenizeSymbolUsages(
    const std::string& line,
    int lineIndex,
    const std::unordered_set<std::string>& parameterSymbols,
    const std::unordered_set<std::string>& localSymbols,
    std::vector<RawToken>& tokens) const {
    static const std::unordered_set<std::string> skipWords = {
        "if", "else", "while", "for", "do", "switch", "case", "default", "break",
        "continue", "return", "try", "catch", "finally", "throw", "match", "await",
        "class", "interface", "annotation", "extends", "implements", "import", "from",
        "as", "new", "this", "super", "null", "true", "false", "function",
        "constructor", "public", "private", "protected", "static", "final", "const",
        "abstract", "async", "value", "int", "float", "string", "bool", "void",
        "object", "Object"
    };

    static const std::regex identifierRegex(R"(\b[A-Za-z_]\w*\b)");
    auto begin = std::sregex_iterator(line.begin(), line.end(), identifierRegex);
    auto end = std::sregex_iterator();

    // Dedupe against tokens already produced on this line. Build the set
    // once from the suffix of `tokens` that belongs to lineIndex (earlier
    // tokenize* passes append in order, so same-line tokens sit at the
    // tail) and look up in O(1). A naive per-identifier linear scan over
    // `tokens` would be O(idents × tokens) per line.
    std::unordered_set<uint64_t> existingTokenStarts;
    for (auto rit = tokens.rbegin(); rit != tokens.rend() && rit->line == lineIndex; ++rit) {
        existingTokenStarts.insert(
            (static_cast<uint64_t>(rit->line) << 32) |
            static_cast<uint64_t>(static_cast<uint32_t>(rit->startChar)));
    }

    for (auto it = begin; it != end; ++it) {
        std::string name = (*it)[0].str();
        if (skipWords.count(name)) continue;

        bool isParameter = parameterSymbols.count(name) > 0;
        bool isLocal = localSymbols.count(name) > 0;
        if (!isParameter && !isLocal) continue;

        int pos = static_cast<int>(it->position());
        int length = static_cast<int>(name.length());
        uint64_t key = (static_cast<uint64_t>(lineIndex) << 32) |
                       static_cast<uint64_t>(static_cast<uint32_t>(pos));
        if (existingTokenStarts.count(key)) continue;

        size_t start = static_cast<size_t>(pos);
        size_t after = start + static_cast<size_t>(length);

        bool afterDot = start > 0 && line[start - 1] == '.';
        bool afterStaticAccess = start >= 2 && line[start - 1] == ':' && line[start - 2] == ':';
        if (afterDot || afterStaticAccess) continue;

        size_t next = after;
        while (next < line.size() && std::isspace(static_cast<unsigned char>(line[next]))) {
            next++;
        }
        if (next < line.size() && line[next] == '(') continue;

        pushToken(tokens, lineIndex, pos, length,
                  encodeTokenType(isParameter ? "parameter" : "variable"), 0);
    }
}

void SemanticTokensHandler::tokenizeMemberAccess(const std::string& line, int lineIndex,
                                                  std::vector<RawToken>& tokens) const {
    auto begin = std::sregex_iterator(line.begin(), line.end(), memberAccessRegex_);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::string memberName = (*it)[2].str();
        int namePos = static_cast<int>(it->position(2));
        size_t afterName = static_cast<size_t>(namePos + static_cast<int>(memberName.length()));

        while (afterName < line.size() && std::isspace(static_cast<unsigned char>(line[afterName]))) {
            afterName++;
        }

        if (afterName < line.size() && line[afterName] == '(') {
            continue;
        }

        pushToken(tokens, lineIndex, namePos, static_cast<int>(memberName.length()),
                  encodeTokenType("property"), 0);
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
