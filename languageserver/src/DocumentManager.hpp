#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

// Include necessary headers
#include "../../mType/token/Token.hpp"
#include "../../mType/lexer/Lexer.hpp"

namespace mtype::lsp {

struct Document {
    std::string uri;
    std::string content;
    int version;

    // Parsed representation
    std::unique_ptr<lexer::Lexer> lexer;
    // Note: Parser and Environment require ImportManager and full semantic analysis
    // For basic LSP features (completion, hover), we only need tokens
    // TODO: Add full semantic analysis when needed for advanced features

    // State
    bool isParsed = false;
    std::vector<std::string> parseErrors;
    std::vector<token::Token> tokens;
};

class DocumentManager {
public:
    void openDocument(const std::string& uri, const std::string& content, int version);
    void updateDocument(const std::string& uri, const std::string& content, int version);
    void closeDocument(const std::string& uri);

    Document* getDocument(const std::string& uri);
    const Document* getDocument(const std::string& uri) const;

    // Parse document
    void parseDocument(const std::string& uri);

    // Query methods
    std::string getWordAtPosition(const std::string& uri, int line, int character) const;
    std::vector<std::string> getIdentifiersInScope(const std::string& uri, int line) const;

private:
    std::unordered_map<std::string, std::unique_ptr<Document>> documents_;

    std::string extractWordAtPosition(const std::string& content, int line, int character) const;
};

} // namespace mtype::lsp
