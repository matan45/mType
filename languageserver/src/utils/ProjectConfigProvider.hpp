#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace mtype::lsp {

/**
 * Lightweight .mtproj parser for the language server.
 * Extracts import search paths and aliases from the project configuration.
 */
class ProjectConfigProvider {
public:
    ProjectConfigProvider() = default;

    /**
     * Find and load the .mtproj file starting from the given workspace root.
     * Searches for .mtproj files in the directory tree.
     * @return true if a .mtproj was found and parsed
     */
    bool loadFromWorkspace(const std::string& workspaceRoot);

    /**
     * Get the absolute search paths resolved from the .mtproj
     */
    const std::vector<std::string>& getSearchPaths() const { return searchPaths_; }

    /**
     * Get the path aliases (e.g., @core -> /abs/path/src/core)
     */
    const std::unordered_map<std::string, std::string>& getAliases() const { return aliases_; }

    /**
     * Get the project root directory (parent of .mtproj file)
     */
    const std::string& getProjectRoot() const { return projectRoot_; }

    /**
     * Check if a project config has been loaded
     */
    bool isLoaded() const { return loaded_; }

    /**
     * Try to resolve an import path using search paths and aliases.
     * Returns the resolved absolute path, or empty string if not found.
     * @param baseDir Directory of the file performing the import
     * @param importPath The import path from the source code
     */
    std::string resolveImport(const std::string& baseDir, const std::string& importPath) const;

private:
    /**
     * Find a .mtproj file by searching the directory tree
     */
    std::string findMtproj(const std::string& startDir) const;

    /**
     * Check if a path is within the workspace root (prevents path traversal)
     */
    bool isWithinWorkspace(const std::string& candidatePath, const std::string& workspaceRoot) const;

    /**
     * Parse the .mtproj file content
     */
    void parseMtproj(const std::string& filePath);

    std::string projectRoot_;
    std::string workspaceRoot_;
    std::vector<std::string> searchPaths_;
    std::unordered_map<std::string, std::string> aliases_;
    bool loaded_ = false;
};

} // namespace mtype::lsp
