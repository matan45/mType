#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include "../../../mType/environment/Environment.hpp"
#include "../DocumentManager.hpp"

namespace mtype::lsp {

// Forward declaration
struct Document;

/**
 * Resolves and parses imported files to extract symbols
 */
class ImportResolver {
public:
    ImportResolver();

    /**
     * Parse all imports for a document and register their symbols
     * @param doc The document whose imports to resolve
     * @param docManager Document manager to access other files
     */
    void resolveImports(Document* doc, DocumentManager* docManager);

    /**
     * Clear the cache of parsed imports
     */
    void clearCache();

private:
    /**
     * Extract import paths from document content
     * @param content The document content
     * @return Vector of relative import paths
     */
    std::vector<std::string> extractImportPaths(const std::string& content);

    /**
     * Resolve a relative import path to an absolute file path
     * @param baseUri The URI of the importing file
     * @param relativePath The relative import path
     * @return Absolute file path
     */
    std::string resolveImportPath(const std::string& baseUri, const std::string& relativePath);

    /**
     * Parse an imported file and extract its symbols
     * @param importPath Absolute path to the imported file
     * @param targetEnv Environment to register symbols into
     * @param targetSymbolLocations Symbol locations map to merge into
     * @param visited Set of already visited files (for circular dependency detection)
     */
    void parseImportedFile(
        const std::string& importPath,
        std::shared_ptr<environment::Environment> targetEnv,
        std::unordered_map<std::string, SymbolLocationInfo>& targetSymbolLocations,
        std::unordered_set<std::string>& visited
    );

    /**
     * URL decode a string (e.g., %3A -> :)
     */
    std::string urlDecode(const std::string& str);

    // Cache of parsed import files (path -> environment with symbols)
    std::unordered_map<std::string, std::shared_ptr<environment::Environment>> importCache_;

    // Cache of symbol locations from imported files (path -> symbol locations)
    std::unordered_map<std::string, std::unordered_map<std::string, SymbolLocationInfo>> symbolLocationCache_;
};

} // namespace mtype::lsp
