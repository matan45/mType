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

        // SECURITY: optional project root(s) for path-traversal containment.
        // When non-empty, all resolved import paths must canonicalize to a
        // location inside one of these directories. When empty (the default
        // — used by ad-hoc scripts and tests), no containment check is
        // performed. Only project builds (ProjectBuilder) opt in.
        // The first entry is the primary project root; additional entries
        // are added for workspace member projects.
        std::vector<std::string> allowedRoots;

        // Current file being processed (for resolving relative imports)
        std::string currentFilePath;

        // Project-level search paths for imports
        std::vector<std::string> searchPaths;

        // Path aliases (e.g., @core -> lib/core)
        std::unordered_map<std::string, std::string> pathAliases;

    public:
        explicit ImportManager();

        ~ImportManager();

        // Set the base directory for relative imports
        void setBaseDirectory(const std::string& dir);

        // SECURITY: opt in to project-root containment for imports. When
        // set, every resolved import must canonicalize to a path inside
        // `root`; otherwise FileException is thrown. Leave unset for
        // ad-hoc scripts where the lib/ directory may live outside the
        // script's own folder.
        void setProjectRoot(const std::string& root);

        // SECURITY: add an additional allowed root for imports. Used by
        // workspace builds to allow cross-project imports between member
        // projects while maintaining path-traversal protection.
        void addAllowedRoot(const std::string& root);

        // Get/Set current file path (for nested import resolution)
        std::string getCurrentFilePath() const;
        void setCurrentFilePath(const std::string& path);

        // Configure project-level import settings
        void setSearchPaths(const std::vector<std::string>& paths);
        void setPathAliases(const std::unordered_map<std::string, std::string>& aliases);

        // Resolve relative and absolute paths
        std::string resolvePath(const std::string& path);

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

    private:
        // Import resolution helpers
        void handleResolvedImport(ast::ImportNode* importNode);
        void handleUnresolvedImport(ast::ImportNode* importNode, const std::string& filePath);
        void resolveImportsInChildren(ast::ASTNode* node);

        // Path resolution and checking helpers
        bool safeCheckInSet(const std::string& rawPath,
                           const std::unordered_set<std::string>& set,
                           bool useConsistentResolve);
        std::filesystem::path normalizeFilePath(const std::filesystem::path& filePath,
                                               bool allowFallback);

        // SECURITY: ensure a resolved path lives inside baseDirectory.
        // Throws errors::FileException on path-traversal attempts. Used by
        // resolvePath() to reject absolute or `..`-laden imports that would
        // otherwise reach files outside the project root. The check uses
        // canonical() + iterator-mismatch on path components so it cannot be
        // bypassed by a sibling directory with the same prefix
        // (e.g. "/projectroot-evil/").
        void enforceWithinProjectRoot(const std::filesystem::path& resolved,
                                      const std::string& originalPath);
    };
}
