#pragma once

#include <cstdint>
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

    // Tokenization passes (each appends to tokens_)
    void tokenizeAnnotations(const std::string& line, int lineIndex);
    void tokenizeClassDeclarations(const std::string& line, int lineIndex);
    void tokenizeInterfaceDeclarations(const std::string& line, int lineIndex);
    void tokenizeMethodDeclarations(const std::string& line, int lineIndex);
    void tokenizeVariableDeclarations(const std::string& line, int lineIndex);
    void tokenizeKeywords(const std::string& line, int lineIndex);
    void tokenizeTypes(const std::string& line, int lineIndex,
                       const std::vector<std::string>& knownClasses);
    void tokenizeModifiers(const std::string& line, int lineIndex);
    void tokenizeFunctionCalls(const std::string& line, int lineIndex);

    // Helpers
    static int encodeTokenType(const std::string& type);
    static int encodeTokenModifiers(const std::vector<std::string>& mods);
    void pushToken(int line, int startChar, int length, int type, int modifiers);

    // Scratch buffer cleared per-request
    std::vector<RawToken> tokens_;

    DocumentManager* documentManager_;
};

} // namespace mtype::lsp
