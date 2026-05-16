#pragma once
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

namespace project::mtclib
{
    /**
     * Resolves library names to .mtcLib file paths.
     * Search order: project libs dir, user libs dir, registry root, MTCPATH env var paths.
     */
    class MtcPathResolver
    {
    public:
        explicit MtcPathResolver(const std::string& projectRoot = ".");

        // Add a flat search path: <dir>/<name>.mtcLib or <dir>/<name>-<version>.mtcLib
        void addSearchPath(const std::string& path);

        // Add a registry-style root: <root>/<name>/<version>/<name>.mtcLib
        void addRegistryRoot(const std::string& path);

        // Resolve a library name to its .mtcLib file path
        std::optional<std::string> resolve(const std::string& libraryName) const;

        // Resolve with version constraint
        std::optional<std::string> resolve(const std::string& libraryName, const std::string& version) const;

        // Get all configured flat search paths
        const std::vector<std::string>& getSearchPaths() const { return searchPaths; }

        // Get all configured registry roots
        const std::vector<std::string>& getRegistryRoots() const { return registryRoots; }

    private:
        std::vector<std::string> searchPaths;
        std::vector<std::string> registryRoots;
        mutable std::unordered_map<std::string, std::string> cache;

        void initializeDefaultPaths(const std::string& projectRoot);
        void loadMtcPathEnv();

        std::optional<std::string> resolveInRegistry(
            const std::string& libraryName, const std::string& version) const;
        std::optional<std::string> resolveInRegistryHighest(
            const std::string& libraryName) const;
    };
}
