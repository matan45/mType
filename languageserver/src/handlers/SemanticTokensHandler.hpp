#pragma once

#include <cstdint>
#include <regex>
#include <string>
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

    // Tokenization passes — each appends to the supplied vector
    void tokenizeAnnotations(const std::string& line, int lineIndex, std::vector<RawToken>& tokens) const;
    void tokenizeClassDeclarations(const std::string& line, int lineIndex, std::vector<RawToken>& tokens) const;
    void tokenizeInterfaceDeclarations(const std::string& line, int lineIndex, std::vector<RawToken>& tokens) const;
    void tokenizeMethodDeclarations(const std::string& line, int lineIndex, std::vector<RawToken>& tokens) const;
    void tokenizeVariableDeclarations(const std::string& line, int lineIndex, std::vector<RawToken>& tokens) const;
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
    std::regex classRegex_;
    std::regex interfaceRegex_;
    std::regex methodRegex_;
    std::regex varRegex_;
    std::regex keywordRegex_;           // single alternation for all keywords
    std::regex modifierRegex_;          // single alternation for all modifiers
    std::regex functionCallRegex_;
    std::regex classDeclLookbehind_;    // for tokenizeTypes skip check

    DocumentManager* documentManager_;
};

} // namespace mtype::lsp
