#pragma once
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <memory>
#include <filesystem>
#include "../ast/ASTNode.hpp"

namespace services
{
    using namespace ast;

    class ImportManager
    {
    private:
        // Track imported files to avoid duplicates
        std::unordered_set<std::string> importedFiles;

        // Track evaluated files to avoid re-evaluation
        std::unordered_set<std::string> evaluatedFiles;

        // Track files currently being evaluated for circular import detection
        std::unordered_set<std::string> beingEvaluated;

        // Cache parsed ASTs
        std::unordered_map<std::string, std::unique_ptr<ASTNode>> astCache;

        // Original base directory (never changes after initialization)
        std::string baseDirectory;

        // Current file being processed (for resolving relative imports)
        std::string currentFilePath;

    public:
        explicit ImportManager();

        ~ImportManager();

        // Set the base directory for relative imports
        void setBaseDirectory(const std::string& dir);

        // Get/Set current file path (for nested import resolution)
        std::string getCurrentFilePath() const { return currentFilePath; }
        void setCurrentFilePath(const std::string& path) { currentFilePath = path; }

        // Resolve relative and absolute paths
        std::string resolvePath(const std::string& path);

        // Resolve path relative to a specific file (for nested imports)
        std::string resolvePathRelativeToFile(const std::string& path, const std::string& relativeToFile);

        // Resolve path consistently relative to base directory (for evaluation tracking)
        std::string resolvePathConsistently(const std::string& path);

        
        // Parse and cache AST only (no evaluation) - for clean architecture
        ASTNode* parseAndCacheAST(const std::string& rawPath);

        // Clear import cache (useful for REPL or hot reload)
        void clearCache();

        // Check if a file has been imported
        bool isImported(const std::string& rawPath);
        
        // Check if a file has been evaluated
        bool isEvaluated(const std::string& rawPath);
        
        // Mark a file as evaluated
        void markAsEvaluated(const std::string& rawPath);
        
        // Check if a file is currently being evaluated (for circular import detection)
        bool isBeingEvaluated(const std::string& rawPath);
        
        // Mark a file as currently being evaluated
        void markAsBeingEvaluated(const std::string& rawPath);
        
        // Mark a file as no longer being evaluated
        void unmarkAsBeingEvaluated(const std::string& rawPath);

        // Get list of imported files
        std::vector<std::string> getImportedFiles() const;

        // Recursively resolve all imports in an AST
        void resolveAllImports(ast::ASTNode* root);
    };
}
