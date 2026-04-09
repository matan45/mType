#include "ImportResolver.hpp"
#include "SymbolRegistrationVisitor.hpp"
#include "../utils/MemoryFileReader.hpp"
#include "../../../mType/lexer/Lexer.hpp"
#include "../../../mType/parser/Parser.hpp"
#include "../../../mType/services/ImportManager.hpp"
#include "../../../mType/environment/EnvironmentBuilder.hpp"
#include <regex>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace mtype::lsp {

ImportResolver::ImportResolver() {
}

void ImportResolver::setProjectConfig(std::shared_ptr<ProjectConfigProvider> config) {
    projectConfig_ = std::move(config);
}

void ImportResolver::resolveImports(Document* doc, DocumentManager* docManager) {
    if (!doc || !doc->environment) {
        return;
    }

    // Extract import paths from document
    auto importPaths = extractImportPaths(doc->content);

    // Track visited files to prevent circular dependencies
    std::unordered_set<std::string> visited;
    visited.insert(doc->uri); // Don't import self

    // Parse each imported file
    for (const auto& relativePath : importPaths) {
        std::string absolutePath;

        // Try project config resolution first (handles search paths and aliases)
        if (projectConfig_ && projectConfig_->isLoaded()) {
            std::string docPath = doc->uri;
            // Remove file:/// prefix
            if (docPath.substr(0, 8) == "file:///") {
                docPath = urlDecode(docPath.substr(8));
                // Windows: /C:/path -> C:/path
                if (docPath.length() >= 3 && docPath[0] == '/' && docPath[2] == ':') {
                    docPath = docPath.substr(1);
                }
            }
            std::string baseDir = fs::path(docPath).parent_path().string();
            absolutePath = projectConfig_->resolveImport(baseDir, relativePath);
        }

        // Fallback: resolve relative to current file
        if (absolutePath.empty()) {
            absolutePath = resolveImportPath(doc->uri, relativePath);
        }

        if (fs::exists(absolutePath)) {
            parseImportedFile(absolutePath, doc->environment, doc->symbolLocations, visited);
        }
    }
}

void ImportResolver::clearCache() {
    importCache_.clear();
    symbolLocationCache_.clear();
}

std::vector<std::string> ImportResolver::extractImportPaths(const std::string& content) {
    std::vector<std::string> paths;

    // Regex to match: import ... from "path"
    std::regex importRegex("import\\s+.*\\s+from\\s+\"([^\"]+)\"");

    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, importRegex)) {
            std::string importPath = match[1].str();
            paths.push_back(importPath);
        }
    }

    return paths;
}

std::string ImportResolver::resolveImportPath(const std::string& baseUri, const std::string& relativePath) {
    // Remove file:/// prefix if present
    std::string path = baseUri;
    if (path.substr(0, 8) == "file:///") {
        path = path.substr(8);
    }

    // URL decode (e.g., %3A -> :)
    path = urlDecode(path);

    // Get directory of the base file
    fs::path basePath(path);
    fs::path baseDir = basePath.parent_path();

    // Resolve relative path
    fs::path targetPath = baseDir / relativePath;
    fs::path normalizedPath = fs::weakly_canonical(targetPath);

    return normalizedPath.string();
}

void ImportResolver::parseImportedFile(
    const std::string& importPath,
    std::shared_ptr<environment::Environment> targetEnv,
    std::unordered_map<std::string, SymbolLocationInfo>& targetSymbolLocations,
    std::unordered_set<std::string>& visited
) {
    // Check if already visited (circular dependency)
    if (visited.find(importPath) != visited.end()) {
        return;
    }

    // Check cache
    if (importCache_.find(importPath) != importCache_.end()) {
        auto cachedEnv = importCache_[importPath];

        // Copy symbols from cached environment to target environment
        if (cachedEnv->getClassRegistry()) {
            for (const auto& className : cachedEnv->getClassRegistry()->getAllItemNames()) {
                auto classDef = cachedEnv->getClassRegistry()->findClass(className);
                if (classDef) {
                    try {
                        targetEnv->registerClass(className, classDef);
                    } catch (...) {
                        // Ignore if already registered
                    }
                }
            }
        }

        if (cachedEnv->getInterfaceRegistry()) {
            auto allInterfaces = cachedEnv->getInterfaceRegistry()->getAllInterfaces();
            for (const auto& [interfaceName, interfaceDef] : allInterfaces) {
                try {
                    targetEnv->registerInterface(interfaceName, interfaceDef);
                } catch (...) {
                    // Ignore if already registered
                }
            }
        }

        if (cachedEnv->getFunctionRegistry()) {
            for (const auto& funcName : cachedEnv->getFunctionRegistry()->getAllItemNames()) {
                auto funcDef = cachedEnv->getFunctionRegistry()->findFunction(funcName);
                if (funcDef) {
                    try {
                        targetEnv->registerFunction(funcName, funcDef);
                    } catch (...) {
                        // Ignore if already registered
                    }
                }
            }
        }

        // Copy symbol locations from cache
        if (symbolLocationCache_.find(importPath) != symbolLocationCache_.end()) {
            const auto& cachedLocations = symbolLocationCache_[importPath];
            for (const auto& [symbolName, location] : cachedLocations) {
                targetSymbolLocations[symbolName] = location;
            }
        }

        return;
    }

    // Mark as visited
    visited.insert(importPath);

    try {
        // Read file content
        std::ifstream file(importPath);
        if (!file.is_open()) {
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        // Create in-memory file reader
        auto fileReader = std::make_unique<MemoryFileReader>();
        fileReader->setContent(importPath, content);

        // Create lexer
        lexer::Lexer lexer(importPath, std::move(fileReader));

        // Create parser
        auto importManager = std::make_unique<services::ImportManager>();
        parser::Parser parser(lexer, std::move(importManager));

        // Parse the file
        auto program = parser.parseProgram();
        if (!program) {
            return;
        }

        // Create temporary environment for this import
        auto importEnv = environment::EnvironmentBuilder::createDefault();

        // Register symbols from the parsed AST
        auto visitor = std::make_unique<SymbolRegistrationVisitor>(importEnv);
        visitor->processProgram(program.get(), importPath);

        // Get symbol locations from the visitor
        const auto& importSymbolLocations = visitor->getSymbolLocations();

        // Cache the environment and symbol locations
        importCache_[importPath] = importEnv;
        symbolLocationCache_[importPath] = importSymbolLocations;

        // Copy symbol locations to target
        for (const auto& [symbolName, location] : importSymbolLocations) {
            targetSymbolLocations[symbolName] = location;
        }

        // Copy symbols to target environment
        if (importEnv->getClassRegistry()) {
            for (const auto& className : importEnv->getClassRegistry()->getAllItemNames()) {
                auto classDef = importEnv->getClassRegistry()->findClass(className);
                if (classDef) {
                    try {
                        targetEnv->registerClass(className, classDef);
                    } catch (const std::exception&) {
                        // Ignore if already registered
                    }
                }
            }
        }

        if (importEnv->getInterfaceRegistry()) {
            auto allInterfaces = importEnv->getInterfaceRegistry()->getAllInterfaces();
            for (const auto& [interfaceName, interfaceDef] : allInterfaces) {
                try {
                    targetEnv->registerInterface(interfaceName, interfaceDef);
                } catch (const std::exception&) {
                    // Ignore if already registered
                }
            }
        }

        if (importEnv->getFunctionRegistry()) {
            for (const auto& funcName : importEnv->getFunctionRegistry()->getAllItemNames()) {
                auto funcDef = importEnv->getFunctionRegistry()->findFunction(funcName);
                if (funcDef) {
                    try {
                        targetEnv->registerFunction(funcName, funcDef);
                    } catch (const std::exception&) {
                        // Ignore if already registered
                    }
                }
            }
        }

        // Recursively resolve imports from this file
        auto nestedImports = extractImportPaths(content);
        for (const auto& nestedRelativePath : nestedImports) {
            std::string nestedAbsolutePath = resolveImportPath("file:///" + importPath, nestedRelativePath);
            if (fs::exists(nestedAbsolutePath)) {
                parseImportedFile(nestedAbsolutePath, targetEnv, targetSymbolLocations, visited);
            }
        }

    } catch (const std::exception&) {
        // Silently ignore parse errors
    }
}

std::string ImportResolver::urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            // Convert hex to char
            int value;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }

    return result;
}

} // namespace mtype::lsp
