#include "MtcPathResolver.hpp"
#include <filesystem>
#include <cstdlib>
#include <sstream>

namespace project::mtclib
{
    MtcPathResolver::MtcPathResolver(const std::string& projectRoot)
    {
        initializeDefaultPaths(projectRoot);
        loadMtcPathEnv();
    }

    void MtcPathResolver::initializeDefaultPaths(const std::string& projectRoot)
    {
        // Project-local libs directory
        auto projectLibs = std::filesystem::path(projectRoot) / "libs";
        searchPaths.push_back(projectLibs.string());

        // User-level libs directory
#ifdef _WIN32
        const char* home = std::getenv("USERPROFILE");
#else
        const char* home = std::getenv("HOME");
#endif
        if (home) {
            auto userLibs = std::filesystem::path(home) / ".mtype" / "libs";
            searchPaths.push_back(userLibs.string());
        }
    }

    void MtcPathResolver::loadMtcPathEnv()
    {
        const char* mtcPath = std::getenv("MTCPATH");
        if (!mtcPath) return;

        std::string pathStr(mtcPath);
        std::istringstream stream(pathStr);
        std::string segment;

#ifdef _WIN32
        char delimiter = ';';
#else
        char delimiter = ':';
#endif

        while (std::getline(stream, segment, delimiter)) {
            if (!segment.empty()) {
                searchPaths.push_back(segment);
            }
        }
    }

    void MtcPathResolver::addSearchPath(const std::string& path)
    {
        searchPaths.push_back(path);
    }

    std::optional<std::string> MtcPathResolver::resolve(const std::string& libraryName) const
    {
        // Check cache first
        auto cacheIt = cache.find(libraryName);
        if (cacheIt != cache.end()) {
            return cacheIt->second;
        }

        for (const auto& searchDir : searchPaths) {
            // Try: {searchDir}/{name}.mtcLib
            auto path = std::filesystem::path(searchDir) / (libraryName + ".mtcLib");
            if (std::filesystem::exists(path)) {
                auto resolved = std::filesystem::canonical(path);
                auto base = std::filesystem::canonical(searchDir);
                // Containment check: ensure resolved path is inside the search directory
                auto resolvedStr = resolved.string();
                auto baseStr = base.string();
                if (resolvedStr.rfind(baseStr, 0) != 0) {
                    continue;  // Path escapes search directory — skip
                }
                cache[libraryName] = resolvedStr;
                return resolvedStr;
            }
        }

        return std::nullopt;
    }

    std::optional<std::string> MtcPathResolver::resolve(
        const std::string& libraryName, const std::string& version) const
    {
        // Check cache
        std::string cacheKey = libraryName + "-" + version;
        auto cacheIt = cache.find(cacheKey);
        if (cacheIt != cache.end()) {
            return cacheIt->second;
        }

        for (const auto& searchDir : searchPaths) {
            // Try versioned: {searchDir}/{name}-{version}.mtcLib
            auto versionedPath = std::filesystem::path(searchDir) / (libraryName + "-" + version + ".mtcLib");
            if (std::filesystem::exists(versionedPath)) {
                std::string resolved = std::filesystem::canonical(versionedPath).string();
                cache[cacheKey] = resolved;
                return resolved;
            }

            // Fall back to unversioned: {searchDir}/{name}.mtcLib
            auto path = std::filesystem::path(searchDir) / (libraryName + ".mtcLib");
            if (std::filesystem::exists(path)) {
                std::string resolved = std::filesystem::canonical(path).string();
                cache[cacheKey] = resolved;
                return resolved;
            }
        }

        return std::nullopt;
    }
}
