#pragma once
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <stack>
#include <memory>
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

        // Stack to detect circular dependencies
        std::stack<std::string> importStack;

        // Cache parsed ASTs
        std::unordered_map<std::string, std::unique_ptr<ASTNode>> astCache;

        // Current working directory for relative imports
        std::string currentDirectory;
        
        // Original base directory (never changes after initialization)
        std::string baseDirectory;

    public:
        explicit ImportManager();

        ~ImportManager();

        // Set the base directory for relative imports
        void setBaseDirectory(const std::string& dir);

        // Resolve relative and absolute paths
        std::string resolvePath(const std::string& path);
        
        // Resolve path consistently relative to base directory (for evaluation tracking)
        std::string resolvePathConsistently(const std::string& path);

        // Check for circular dependencies
        bool wouldCauseCircularDependency(const std::string& filePath);

        // Get circular dependency chain for error message
        std::string getCircularDependencyChain(const std::string& filePath);

        // Import a file
        ASTNode* importFile(const std::string& rawPath);
        
        // Clear import cache (useful for REPL or hot reload)
        void clearCache();

        // Check if a file has been imported
        bool isImported(const std::string& rawPath);
        
        // Check if a file has been evaluated
        bool isEvaluated(const std::string& rawPath);
        
        // Mark a file as evaluated
        void markAsEvaluated(const std::string& rawPath);

        // Get list of imported files
        std::vector<std::string> getImportedFiles() const;
    };
}
