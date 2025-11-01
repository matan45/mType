#include "DocumentManager.hpp"
#include "../../mType/token/TokenType.hpp"
#include "../../mType/environment/EnvironmentBuilder.hpp"
#include "utils/MemoryFileReader.hpp"
#include "analysis/SymbolRegistrationVisitor.hpp"
#include "analysis/ImportResolver.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>
#include <regex>

using namespace lexer;
using namespace token;
using namespace parser;
using namespace environment;
using namespace services;

namespace mtype::lsp {

DocumentManager::DocumentManager() {
    // ImportManager is now per-document for LSP
    importResolver_ = std::make_unique<ImportResolver>();
}

DocumentManager::~DocumentManager() = default;

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
    std::cerr << "[DocumentManager::parseDocument] Parsing document: " << uri << std::endl;
    auto doc = getDocument(uri);
    if (!doc) {
        std::cerr << "[DocumentManager::parseDocument] Document not found!" << std::endl;
        return;
    }

    try {
        // Clear errors but preserve AST and environment if parse fails
        doc->parseErrors.clear();
        doc->semanticErrors.clear();
        doc->tokens.clear();

        // Save previous AST and environment in case parse fails
        std::vector<std::unique_ptr<ast::ASTNode>> previousAst;
        std::shared_ptr<environment::Environment> previousEnvironment;
        std::unordered_map<std::string, SymbolLocationInfo> previousSymbolLocations;

        if (!doc->ast.empty()) {
            // Keep a backup of the previous valid state
            std::cerr << "[DocumentManager::parseDocument] Backing up previous AST with " << doc->ast.size() << " nodes" << std::endl;
            // We can't move, so we'll just keep the old one if parsing fails
        }

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

        // Try to parse the document to get new AST
        std::vector<std::unique_ptr<ast::ASTNode>> newAst;
        bool parseSucceeded = false;

        try {
            auto program = doc->parser->parseProgram();
            if (program) {
                newAst.push_back(std::move(program));
                parseSucceeded = true;
                std::cerr << "[DocumentManager::parseDocument] AST parsed successfully" << std::endl;
            } else {
                std::cerr << "[DocumentManager::parseDocument] Parser returned null program" << std::endl;
            }
        } catch (const std::exception& e) {
            doc->parseErrors.push_back(e.what());
            std::cerr << "[DocumentManager::parseDocument] Parse error: " << e.what() << std::endl;
            std::cerr << "[DocumentManager::parseDocument] Keeping previous AST and environment for autocomplete" << std::endl;
        }

        // Only update AST and environment if parsing succeeded
        if (parseSucceeded && !newAst.empty()) {
            doc->ast = std::move(newAst);

            // Create environment for semantic analysis
            doc->environment = EnvironmentBuilder::createDefault();
            std::cerr << "[DocumentManager::parseDocument] Environment created" << std::endl;
        } else if (doc->environment) {
            std::cerr << "[DocumentManager::parseDocument] Reusing previous environment with registered symbols" << std::endl;

            // Log what's in the registries from previous parse
            auto classRegistry = doc->environment->getClassRegistry();
            auto interfaceRegistry = doc->environment->getInterfaceRegistry();
            if (classRegistry) {
                auto classNames = classRegistry->getAllItemNames();
                std::cerr << "[DocumentManager::parseDocument] Reused class registry has " << classNames.size() << " classes" << std::endl;
                for (const auto& className : classNames) {
                    std::cerr << "[DocumentManager::parseDocument]   - Class: " << className << std::endl;
                }
            }
            if (interfaceRegistry) {
                auto interfaces = interfaceRegistry->getAllInterfaces();
                std::cerr << "[DocumentManager::parseDocument] Reused interface registry has " << interfaces.size() << " interfaces" << std::endl;
                for (const auto& [interfaceName, interfaceDef] : interfaces) {
                    std::cerr << "[DocumentManager::parseDocument]   - Interface: " << interfaceName << std::endl;
                }
            }
        } else {
            // First parse and it failed - create empty environment
            doc->environment = EnvironmentBuilder::createDefault();
            std::cerr << "[DocumentManager::parseDocument] First parse failed - creating empty environment" << std::endl;
        }

        // Build symbol tables from AST using symbol registration (only if we have new AST)
        try {
            if (parseSucceeded && !doc->ast.empty()) {
                std::cerr << "[DocumentManager::parseDocument] Starting symbol registration for " << doc->ast.size() << " AST nodes" << std::endl;

                // Create symbol registration visitor
                auto visitor = std::make_unique<SymbolRegistrationVisitor>(doc->environment);

                // Traverse AST to register all symbols
                for (const auto& node : doc->ast) {
                    if (node) {
                        std::cerr << "[DocumentManager::parseDocument] Processing AST node" << std::endl;
                        visitor->processProgram(node.get(), uri);
                    }
                }

                // Store symbol locations for go-to-definition
                doc->symbolLocations = visitor->getSymbolLocations();
                std::cerr << "[DocumentManager::parseDocument] Symbol registration complete. Registered " << doc->symbolLocations.size() << " symbols" << std::endl;

                // Log what's in the registries
                auto classRegistry = doc->environment->getClassRegistry();
                auto interfaceRegistry = doc->environment->getInterfaceRegistry();
                if (classRegistry) {
                    auto classNames = classRegistry->getAllItemNames();
                    std::cerr << "[DocumentManager::parseDocument] Class registry has " << classNames.size() << " classes" << std::endl;
                    for (const auto& className : classNames) {
                        std::cerr << "[DocumentManager::parseDocument]   - Class: " << className << std::endl;
                    }
                }
                if (interfaceRegistry) {
                    auto interfaces = interfaceRegistry->getAllInterfaces();
                    std::cerr << "[DocumentManager::parseDocument] Interface registry has " << interfaces.size() << " interfaces" << std::endl;
                    for (const auto& [interfaceName, interfaceDef] : interfaces) {
                        std::cerr << "[DocumentManager::parseDocument]   - Interface: " << interfaceName << std::endl;
                    }
                }
            } else {
                std::cerr << "[DocumentManager::parseDocument] Parse failed or AST empty, skipping new symbol registration (keeping previous symbols if any)" << std::endl;
            }
        } catch (const std::exception& e) {
            doc->semanticErrors.push_back(std::string("Symbol registration error: ") + e.what());
            std::cerr << "[DocumentManager::parseDocument] Symbol registration error: " << e.what() << std::endl;
        }

        // Resolve and parse imported files to get their symbols
        if (doc->environment && importResolver_) {
            try {
                std::cerr << "[DocumentManager::parseDocument] Resolving imports..." << std::endl;
                importResolver_->resolveImports(doc, this);
                std::cerr << "[DocumentManager::parseDocument] Import resolution complete" << std::endl;

                // Log symbols after import resolution
                auto classRegistry = doc->environment->getClassRegistry();
                auto interfaceRegistry = doc->environment->getInterfaceRegistry();
                if (classRegistry) {
                    auto classNames = classRegistry->getAllItemNames();
                    std::cerr << "[DocumentManager::parseDocument] After imports - Class registry has " << classNames.size() << " classes" << std::endl;
                    for (const auto& className : classNames) {
                        std::cerr << "[DocumentManager::parseDocument]   - Class: " << className << std::endl;
                    }
                }
                if (interfaceRegistry) {
                    auto interfaces = interfaceRegistry->getAllInterfaces();
                    std::cerr << "[DocumentManager::parseDocument] After imports - Interface registry has " << interfaces.size() << " interfaces" << std::endl;
                    for (const auto& [interfaceName, interfaceDef] : interfaces) {
                        std::cerr << "[DocumentManager::parseDocument]   - Interface: " << interfaceName << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "[DocumentManager::parseDocument] Import resolution error: " << e.what() << std::endl;
            }
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

    std::cerr << "[DocumentManager::findDefinition] Called for uri: " << uri
              << " at line: " << line << ", character: " << character << std::endl;

    auto doc = getDocument(uri);
    if (!doc || !doc->isParsed || !doc->environment) {
        std::cerr << "[DocumentManager::findDefinition] Document not parsed or environment missing" << std::endl;
        return std::nullopt;
    }

    // Get the current line to check for method calls
    std::istringstream stream(doc->content);
    std::string currentLine;
    int currentLineNum = 0;
    while (std::getline(stream, currentLine) && currentLineNum < line) {
        currentLineNum++;
    }

    std::cerr << "[DocumentManager::findDefinition] Current line: " << currentLine << std::endl;

    // Get the symbol name at the position
    std::string symbolName = extractWordAtPosition(doc->content, line, character);
    if (symbolName.empty()) {
        std::cerr << "[DocumentManager::findDefinition] No symbol found at position" << std::endl;
        return std::nullopt;
    }

    std::cerr << "[DocumentManager::findDefinition] Symbol name: " << symbolName << std::endl;

    // Check if this is a method call (e.g., "variable.methodName(...)")
    // Look backwards from the character position to see if there's a dot
    if (character > 0 && currentLineNum == line) {
        int dotPos = -1;

        // First, look backwards through the current identifier and any whitespace to find a dot
        for (int i = character - 1; i >= 0; i--) {
            if (currentLine[i] == '.') {
                dotPos = i;
                break;
            } else if (!std::isalnum(currentLine[i]) && currentLine[i] != '_' && !std::isspace(currentLine[i])) {
                // Found a character that's not part of an identifier, whitespace, or a dot
                // This means we're not in a method call
                break;
            }
        }

        if (dotPos != -1) {
            std::cerr << "[DocumentManager::findDefinition] Detected method call (found dot at position " << dotPos << ")" << std::endl;

            // This is a method call: extract the variable name before the dot
            int varStart = dotPos - 1;
            while (varStart >= 0 && (std::isalnum(currentLine[varStart]) || currentLine[varStart] == '_')) {
                varStart--;
            }
            varStart++;

            std::string varName = currentLine.substr(varStart, dotPos - varStart);
            std::cerr << "[DocumentManager::findDefinition] Variable name: " << varName << std::endl;

            // Infer the type of this variable by scanning the document
            std::string varType = inferVariableType(doc->content, varName);

            if (!varType.empty()) {
                // Look for ClassName.methodName in symbol locations
                std::string methodKey = varType + "." + symbolName;
                std::cerr << "[DocumentManager::findDefinition] Looking for method key: " << methodKey << std::endl;

                auto it = doc->symbolLocations.find(methodKey);
                if (it != doc->symbolLocations.end()) {
                    const auto& symbolLoc = it->second;
                    std::cerr << "[DocumentManager::findDefinition] Found method definition at "
                              << symbolLoc.uri << ":" << symbolLoc.line << ":" << symbolLoc.column << std::endl;
                    SymbolLocation result;
                    result.uri = symbolLoc.uri;
                    result.line = symbolLoc.line;
                    result.column = symbolLoc.column;
                    return result;
                } else {
                    std::cerr << "[DocumentManager::findDefinition] Method key not found in symbol locations" << std::endl;
                }
            }
        }
    }

    // Fall back to looking up the symbol directly (for classes, interfaces, functions)
    std::cerr << "[DocumentManager::findDefinition] Falling back to direct symbol lookup" << std::endl;
    auto it = doc->symbolLocations.find(symbolName);
    if (it != doc->symbolLocations.end()) {
        const auto& symbolLoc = it->second;
        std::cerr << "[DocumentManager::findDefinition] Found symbol at "
                  << symbolLoc.uri << ":" << symbolLoc.line << ":" << symbolLoc.column << std::endl;
        SymbolLocation result;
        result.uri = symbolLoc.uri;
        result.line = symbolLoc.line;
        result.column = symbolLoc.column;
        return result;
    }

    std::cerr << "[DocumentManager::findDefinition] Symbol not found: " << symbolName << std::endl;
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

std::string DocumentManager::inferVariableType(const std::string& content, const std::string& varName) const {
    std::cerr << "[DocumentManager::inferVariableType] Inferring type for variable: " << varName << std::endl;

    // Pattern 1: ClassName<GenericType> varName = new ClassName<GenericType>(...)
    // Pattern 2: ClassName<GenericType> varName = ...
    // Note: <GenericType> is optional, handles both generic and non-generic types
    // Captures only the base class name without generic parameters
    std::regex declPattern1("([A-Z][a-zA-Z0-9_]*)(?:<[^>]+>)?\\s+" + varName + "\\s*=\\s*new\\s+([A-Z][a-zA-Z0-9_]*)");
    std::regex declPattern2("([A-Z][a-zA-Z0-9_]*)(?:<[^>]+>)?\\s+" + varName + "\\s*=");

    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        std::smatch match;

        // Try pattern 1: with "new" keyword
        if (std::regex_search(line, match, declPattern1)) {
            // Prefer the type from "new" if it matches the declaration type
            std::string declType = match[1].str();
            std::string newType = match[2].str();
            std::cerr << "[DocumentManager::inferVariableType] Found declaration: "
                      << declType << " " << varName << " = new " << newType << std::endl;
            return newType; // Return the type from "new" as it's the actual instantiated type
        }

        // Try pattern 2: simple declaration
        if (std::regex_search(line, match, declPattern2)) {
            std::string declType = match[1].str();
            std::cerr << "[DocumentManager::inferVariableType] Found declaration: "
                      << declType << " " << varName << std::endl;
            return declType;
        }
    }

    std::cerr << "[DocumentManager::inferVariableType] Could not infer type for: " << varName << std::endl;
    return ""; // Type not found
}

} // namespace mtype::lsp
