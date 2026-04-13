#pragma once
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

namespace project::mtclib
{
    /**
     * Resolves library names to .mtcLib file paths.
     * Search order: project libs dir, user libs dir, MTCPATH env var paths.
     */
    class MtcPathResolver
    {
    public:
        explicit MtcPathResolver(const std::string& projectRoot = ".");

        // Add a search path
        void addSearchPath(const std::string& path);

        // Resolve a library name to its .mtcLib file path
        std::optional<std::string> resolve(const std::string& libraryName) const;

        // Resolve with version constraint
        std::optional<std::string> resolve(const std::string& libraryName, const std::string& version) const;

        // Get all configured search paths
        const std::vector<std::string>& getSearchPaths() const { return searchPaths; }

    private:
        std::vector<std::string> searchPaths;
        mutable std::unordered_map<std::string, std::string> cache;

        void initializeDefaultPaths(const std::string& projectRoot);
        void loadMtcPathEnv();
    };
}
