#include "DocumentManager.hpp"
#include "../../mType/token/TokenType.hpp"
#include "../../mType/environment/EnvironmentBuilder.hpp"
#include "utils/MemoryFileReader.hpp"
#include <sstream>
#include <algorithm>

using namespace lexer;
using namespace token;
using namespace parser;
using namespace environment;
using namespace services;

namespace mtype::lsp {

DocumentManager::DocumentManager() {
    // ImportManager is now per-document for LSP
}

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
        // Clear previous state
        doc->parseErrors.clear();
        doc->semanticErrors.clear();
        doc->tokens.clear();
        doc->ast.clear();

        // Create an in-memory file reader for LSP
        auto fileReader = std::make_unique<MemoryFileReader>();
        fileReader->setContent(uri, doc->content);

        // Create lexer with our custom file reader
        doc->lexer = std::make_unique<Lexer>(uri, std::move(fileReader));

        // Note: Parser takes ownership of ImportManager, but we need to keep a reference
        // For LSP, we'll use a per-document ImportManager for now
        auto docImportManager = std::make_unique<ImportManager>();

        // Create parser with the lexer
        doc->parser = std::make_unique<Parser>(*doc->lexer, std::move(docImportManager));

        // Parse the document to get AST
        try {
            auto program = doc->parser->parseProgram();
            if (program) {
                doc->ast.push_back(std::move(program));
            }
        } catch (const std::exception& e) {
            doc->parseErrors.push_back(e.what());
        }

        // Create environment for semantic analysis
        doc->environment = EnvironmentBuilder::createDefault();

        // Build symbol tables from AST
        // Note: Full AST traversal and symbol registration would happen here
        // For now, we just have the environment ready for basic queries
        try {
            // TODO: Implement AST visitor to register classes, functions, variables
            // This would involve traversing the AST and calling:
            // - environment->registerClass() for class definitions
            // - environment->registerFunction() for function definitions
            // - environment->declareVariable() for variable declarations
        } catch (const std::exception& e) {
            doc->semanticErrors.push_back(e.what());
        }

        // Tokenize for token-based features (keep for completion)
        // Re-create lexer for tokenization
        auto tokenFileReader = std::make_unique<MemoryFileReader>();
        tokenFileReader->setContent(uri, doc->content);
        auto tokenLexer = std::make_unique<Lexer>(uri, std::move(tokenFileReader));

        Token token;
        do {
            token = tokenLexer->getNextToken();
            doc->tokens.push_back(token);
        } while (token.type != TokenType::END);

        doc->isParsed = true;

    } catch (const std::exception& e) {
        doc->parseErrors.push_back(std::string("Fatal error: ") + e.what());
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

    // If we have an environment, use it for more accurate scope information
    if (doc->environment && doc->environment->getVariableManager()) {
        // Get variables from environment
        auto vars = doc->environment->getVariableManager()->getAllVariableNames();
        identifiers.insert(identifiers.end(), vars.begin(), vars.end());
    } else {
        // Fallback: Extract identifiers from tokens
        for (const auto& token : doc->tokens) {
            if (token.type == TokenType::IDENTIFIER) {
                // Avoid duplicates
                std::string tokenStr = token.stringValue.getString();
                if (std::find(identifiers.begin(), identifiers.end(), tokenStr) == identifiers.end()) {
                    identifiers.push_back(tokenStr);
                }
            }
        }
    }

    return identifiers;
}

std::optional<DocumentManager::SymbolLocation> DocumentManager::findDefinition(
    const std::string& uri, int line, int character) const {

    auto doc = getDocument(uri);
    if (!doc || !doc->isParsed || !doc->environment) {
        return std::nullopt;
    }

    // Get the symbol name at the position
    std::string symbolName = extractWordAtPosition(doc->content, line, character);
    if (symbolName.empty()) {
        return std::nullopt;
    }

    // Try to find the symbol in the environment
    // This is a simplified implementation - a full implementation would need to:
    // 1. Determine the symbol type (variable, function, class, etc.)
    // 2. Search the appropriate scope
    // 3. Handle imported symbols

    // For now, we'll search through the AST to find class/function definitions
    // A more robust implementation would use a symbol table with location information

    // TODO: Implement proper symbol table with source locations
    // For now, return nullopt as placeholder
    return std::nullopt;
}

std::optional<std::string> DocumentManager::getTypeInfo(
    const std::string& uri, int line, int character) const {

    auto doc = getDocument(uri);
    if (!doc || !doc->isParsed || !doc->environment) {
        return std::nullopt;
    }

    // Get the symbol name at the position
    std::string symbolName = extractWordAtPosition(doc->content, line, character);
    if (symbolName.empty()) {
        return std::nullopt;
    }

    // Try to get type information from environment
    try {
        auto varDef = doc->environment->findVariable(symbolName);
        if (varDef) {
            // TODO: Extract actual type information from VariableDefinition
            // For now, just return that it's a variable
            return "variable: " + symbolName;
        }
    } catch (...) {
        // Ignore errors
    }

    return std::nullopt;
}

std::vector<DocumentManager::SymbolInfo> DocumentManager::getDocumentSymbols(const std::string& uri) const {
    std::vector<SymbolInfo> symbols;

    auto doc = getDocument(uri);
    if (!doc || !doc->isParsed) {
        return symbols;
    }

    // TODO: Traverse AST to extract symbol information
    // For now, return empty vector as placeholder
    // A full implementation would:
    // 1. Traverse the AST
    // 2. Extract class, function, variable definitions
    // 3. Record their locations and kinds

    return symbols;
}

} // namespace mtype::lsp
