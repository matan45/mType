#pragma once

#include <cstdint>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>
#include "../utils/LSPTypes.hpp"
#include "../DocumentManager.hpp"

namespace mtype::lsp {

struct SemanticTokens {
    std::vector<uint32_t> data;  // delta-encoded: [deltaLine, deltaStart, length, tokenType, tokenModifiers]

    json toJson() const {
        return json{{"data", data}};
    }
};

class SemanticTokensHandler {
public:
    explicit SemanticTokensHandler(DocumentManager* docMgr);

    SemanticTokens handleSemanticTokensFull(const std::string& uri);

    // Legend arrays — must match exactly what we register in capabilities.
    // The order determines the integer encoding.
    static const std::vector<std::string>& tokenTypes();
    static const std::vector<std::string>& tokenModifiers();

private:
    // One raw token before delta-encoding
    struct RawToken {
        int line;
        int startChar;
        int length;
        int tokenType;
        int tokenModifiers;
    };

    // Per-document state tracked while walking lines.
    struct TokenizationState {
        bool pendingClassBody = false;
        bool pendingFunctionBody = false;
        int classDepth = 0;
        int functionDepth = 0;
        std::unordered_set<std::string> localSymbols;
        std::unordered_set<std::string> parameterSymbols;
    };

    std::vector<std::string> collectKnownClasses(const std::string& uri) const;
    void processSourceLine(const std::string& rawLine, int lineIndex,
                           const std::vector<std::string>& knownClasses,
                           TokenizationState& state,
                           std::vector<RawToken>& tokens);
    static void updateScopeDepth(const std::string& semanticLine,
                                 TokenizationState& state);
    static SemanticTokens deltaEncodeTokens(std::vector<RawToken>& tokens);

    // Per-line lexer helpers — extracted from handleSemanticTokensFull so
    // the outer driver stays small and these stay independently testable.
    static int countBraces(const std::string& sourceLine, char brace);
    static size_t findLineCommentStart(const std::string& sourceLine);
    // Forward state machine: replaces look-behind `prev == '\\'` guard so
    // an escaped backslash like "\\" doesn't fool the closing-quote check.
    static std::string maskStringLiterals(const std::string& sourceLine,
                                          int currentLine,
                                          std::vector<RawToken>& rawTokens);

    // Tokenization passes — each appends to the supplied vector
    void tokenizeAnnotations(const std::string& line, int lineIndex, std::vector<RawToken>& tokens) const;
    void tokenizeLineComment(const std::string& line, int lineIndex, size_t commentStart, std::vector<RawToken>& tokens) const;
    void tokenizeClassDeclarations(const std::string& line, int lineIndex, std::vector<RawToken>& tokens) const;
    // `annotation Foo { ... }` declarations — colors both the
    // `annotation` keyword and the type name. Mirrors the
    // class/interface declaration passes.
    void tokenizeAnnotationDeclarations(const std::string& line, int lineIndex, std::vector<RawToken>& tokens) const;
    void tokenizeInterfaceDeclarations(const std::string& line, int lineIndex, std::vector<RawToken>& tokens) const;
    void tokenizeMethodDeclarations(const std::string& line, int lineIndex, std::vector<RawToken>& tokens) const;
    void tokenizeParameters(const std::string& line, int lineIndex,
                            std::unordered_set<std::string>& parameterSymbols,
                            std::vector<RawToken>& tokens) const;
    void tokenizeVariableDeclarations(const std::string& line, int lineIndex,
                                      bool classMemberContext,
                                      std::unordered_set<std::string>& localSymbols,
                                      std::vector<RawToken>& tokens) const;
    void tokenizeMemberAccess(const std::string& line, int lineIndex, std::vector<RawToken>& tokens) const;
    void tokenizeSymbolUsages(const std::string& line, int lineIndex,
                              const std::unordered_set<std::string>& parameterSymbols,
                              const std::unordered_set<std::string>& localSymbols,
                              std::vector<RawToken>& tokens) const;
    void tokenizeKeywords(const std::string& line, int lineIndex, std::vector<RawToken>& tokens) const;
    void tokenizeTypes(const std::string& line, int lineIndex,
                       const std::vector<std::string>& knownClasses,
                       std::vector<RawToken>& tokens) const;
    void tokenizeModifiers(const std::string& line, int lineIndex, std::vector<RawToken>& tokens) const;
    void tokenizeFunctionCalls(const std::string& line, int lineIndex, std::vector<RawToken>& tokens) const;

    // Helpers
    static int encodeTokenType(const std::string& type);
    static int encodeTokenModifiers(const std::vector<std::string>& mods);
    static void pushToken(std::vector<RawToken>& tokens, int line, int startChar, int length, int type, int modifiers);

    // Pre-compiled regexes (built once in constructor)
    std::regex annotationRegex_;
    std::regex annotationDeclRegex_;
    std::regex classRegex_;
    std::regex interfaceRegex_;
    std::regex methodRegex_;
    std::regex parameterRegex_;
    std::regex varRegex_;
    std::regex memberAccessRegex_;
    std::regex keywordRegex_;           // single alternation for all keywords
    std::regex modifierRegex_;          // single alternation for all modifiers
    std::regex functionCallRegex_;
    std::regex classDeclLookbehind_;    // for tokenizeTypes skip check

    DocumentManager* documentManager_;
};

} // namespace mtype::lsp
