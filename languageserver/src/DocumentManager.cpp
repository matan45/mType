#include "DocumentManager.hpp"
#include "../../mType/token/TokenType.hpp"
#include "utils/MemoryFileReader.hpp"
#include <sstream>
#include <algorithm>

using namespace lexer;
using namespace token;

namespace mtype::lsp {

void DocumentManager::openDocument(const std::string& uri, const std::string& content, int version) {
    auto doc = std::make_unique<Document>();
    doc->uri = uri;
    doc->content = content;
    doc->version = version;

    documents_[uri] = std::move(doc);
    parseDocument(uri);
}

void DocumentManager::updateDocument(const std::string& uri, const std::string& content, int version) {
    auto it = documents_.find(uri);
    if (it != documents_.end()) {
        it->second->content = content;
        it->second->version = version;
        it->second->isParsed = false;
        parseDocument(uri);
    }
}

void DocumentManager::closeDocument(const std::string& uri) {
    documents_.erase(uri);
}

Document* DocumentManager::getDocument(const std::string& uri) {
    auto it = documents_.find(uri);
    return it != documents_.end() ? it->second.get() : nullptr;
}

const Document* DocumentManager::getDocument(const std::string& uri) const {
    auto it = documents_.find(uri);
    return it != documents_.end() ? it->second.get() : nullptr;
}

void DocumentManager::parseDocument(const std::string& uri) {
    auto doc = getDocument(uri);
    if (!doc) return;

    try {
        // Create an in-memory file reader for LSP
        auto fileReader = std::make_unique<MemoryFileReader>();
        fileReader->setContent(uri, doc->content);

        // Create lexer with our custom file reader
        doc->lexer = std::make_unique<Lexer>(uri, std::move(fileReader));

        // Tokenize all input (get all tokens)
        doc->tokens.clear();
        Token token;
        do {
            token = doc->lexer->getNextToken();
            doc->tokens.push_back(token);
        } while (token.type != TokenType::END);

        // Note: For basic LSP features, we only need tokens
        // Full semantic analysis (Parser + Environment) will be added later
        // when implementing advanced features like semantic diagnostics

        doc->isParsed = true;
        doc->parseErrors.clear();

    } catch (const std::exception& e) {
        doc->parseErrors.push_back(e.what());
        doc->isParsed = false;
    }
}

std::string DocumentManager::getWordAtPosition(const std::string& uri, int line, int character) const {
    auto doc = getDocument(uri);
    if (!doc) return "";

    return extractWordAtPosition(doc->content, line, character);
}

std::string DocumentManager::extractWordAtPosition(const std::string& content, int line, int character) const {
    std::istringstream stream(content);
    std::string currentLine;
    int currentLineNum = 0;

    // Find the line
    while (std::getline(stream, currentLine) && currentLineNum < line) {
        currentLineNum++;
    }

    if (currentLineNum != line || character >= static_cast<int>(currentLine.length())) {
        return "";
    }

    // Find word boundaries
    int start = character;
    int end = character;

    // Move start backward
    while (start > 0 && (std::isalnum(currentLine[start - 1]) || currentLine[start - 1] == '_')) {
        start--;
    }

    // Move end forward
    while (end < static_cast<int>(currentLine.length()) &&
           (std::isalnum(currentLine[end]) || currentLine[end] == '_')) {
        end++;
    }

    return currentLine.substr(start, end - start);
}

std::vector<std::string> DocumentManager::getIdentifiersInScope(const std::string& uri, int line) const {
    auto doc = getDocument(uri);
    if (!doc || !doc->isParsed) {
        return {};
    }

    std::vector<std::string> identifiers;

    // Extract identifiers from tokens
    for (const auto& token : doc->tokens) {
        if (token.type == TokenType::IDENTIFIER) {
            // Avoid duplicates
            std::string tokenStr = token.stringValue.getString();
            if (std::find(identifiers.begin(), identifiers.end(), tokenStr) == identifiers.end()) {
                identifiers.push_back(tokenStr);
            }
        }
    }

    return identifiers;
}

} // namespace mtype::lsp
