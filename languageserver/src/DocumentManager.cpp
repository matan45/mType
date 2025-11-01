#include "DocumentManager.hpp"
#include "../../mType/token/TokenType.hpp"
#include "../../mType/environment/EnvironmentBuilder.hpp"
#include "utils/MemoryFileReader.hpp"
#include "analysis/SymbolRegistrationVisitor.hpp"
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

        // Build symbol tables from AST using symbol registration
        try {
            if (!doc->ast.empty()) {
                // Create symbol registration visitor
                auto visitor = std::make_unique<SymbolRegistrationVisitor>(doc->environment);

                // Traverse AST to register all symbols
                for (const auto& node : doc->ast) {
                    if (node) {
                        visitor->processProgram(node.get(), uri);
                    }
                }

                // Store symbol locations for go-to-definition
                doc->symbolLocations = visitor->getSymbolLocations();
            }
        } catch (const std::exception& e) {
            doc->semanticErrors.push_back(std::string("Symbol registration error: ") + e.what());
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

    // Look up the symbol in our symbol locations map
    auto it = doc->symbolLocations.find(symbolName);
    if (it != doc->symbolLocations.end()) {
        const auto& symbolLoc = it->second;
        SymbolLocation result;
        result.uri = symbolLoc.uri;
        result.line = symbolLoc.line;
        result.column = symbolLoc.column;
        return result;
    }

    // Symbol not found in registered symbols
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
        // Check for variables
        auto varDef = doc->environment->findVariable(symbolName);
        if (varDef) {
            std::string typeInfo = symbolName + ": ";

            // Get the value type
            auto valueType = varDef->getType();
            switch (valueType) {
                case value::ValueType::INT:
                    typeInfo += "int";
                    break;
                case value::ValueType::FLOAT:
                    typeInfo += "float";
                    break;
                case value::ValueType::BOOL:
                    typeInfo += "bool";
                    break;
                case value::ValueType::STRING:
                    typeInfo += "string";
                    break;
                case value::ValueType::OBJECT: {
                    const auto& className = varDef->getClassName();
                    if (!className.empty()) {
                        typeInfo += className;
                    } else {
                        typeInfo += "object";
                    }
                    break;
                }
                case value::ValueType::VOID:
                    typeInfo += "void";
                    break;
                default:
                    typeInfo += "unknown";
                    break;
            }

            return typeInfo;
        }

        // Check for functions
        if (doc->environment->getFunctionRegistry()) {
            auto funcDef = doc->environment->getFunctionRegistry()->findFunction(symbolName);
            if (funcDef) {
                return "function " + symbolName + "()";
            }
        }

        // Check for classes
        if (doc->environment->getClassRegistry()) {
            if (doc->environment->getClassRegistry()->hasClass(symbolName)) {
                return "class " + symbolName;
            }
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

    // Extract symbols from the symbol locations map populated during parsing
    for (const auto& [symbolName, location] : doc->symbolLocations) {
        SymbolInfo info;
        info.name = symbolName;
        info.line = location.line;
        info.column = location.column;

        // Determine symbol kind by checking the environment
        try {
            // Check if it's a class
            if (doc->environment && doc->environment->getClassRegistry()) {
                if (doc->environment->getClassRegistry()->hasClass(symbolName)) {
                    info.kind = "class";
                    symbols.push_back(info);
                    continue;
                }
            }

            // Check if it's a function
            if (doc->environment && doc->environment->getFunctionRegistry()) {
                auto funcDef = doc->environment->getFunctionRegistry()->findFunction(symbolName);
                if (funcDef) {
                    info.kind = "function";
                    symbols.push_back(info);
                    continue;
                }
            }

            // Check if it's a variable
            if (doc->environment) {
                auto varDef = doc->environment->findVariable(symbolName);
                if (varDef) {
                    info.kind = "variable";
                    symbols.push_back(info);
                    continue;
                }
            }

            // Default to "symbol" if we can't determine the kind
            info.kind = "symbol";
            symbols.push_back(info);

        } catch (...) {
            // Ignore errors, skip this symbol
        }
    }

    return symbols;
}

} // namespace mtype::lsp
