#include "MtcPathResolver.hpp"
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <vector>

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

            // mtpm package registry (populated by `mtpm install`).
            // Layout: <root>/<name>/<version>/<name>.mtcLib
            auto userRegistry = std::filesystem::path(home) / ".mtype" / "registry";
            registryRoots.push_back(userRegistry.string());
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

    void MtcPathResolver::addRegistryRoot(const std::string& path)
    {
        registryRoots.push_back(path);
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

        // Fall back to registry layout: pick the highest version available.
        auto registryHit = resolveInRegistryHighest(libraryName);
        if (registryHit) {
            cache[libraryName] = *registryHit;
        }
        return registryHit;
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

        // Registry layout takes priority when a version is requested —
        // <root>/<name>/<version>/<name>.mtcLib is the only place version-pinned
        // builds land.
        auto registryHit = resolveInRegistry(libraryName, version);
        if (registryHit) {
            cache[cacheKey] = *registryHit;
            return registryHit;
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

    std::optional<std::string> MtcPathResolver::resolveInRegistry(
        const std::string& libraryName, const std::string& version) const
    {
        // Version may be a range like "^0.0.1" — first try it literally, then strip
        // common range prefixes ('^', '~', '=', '>=') and retry with the bare version.
        auto tryVersion = [&](const std::string& v) -> std::optional<std::string> {
            for (const auto& root : registryRoots) {
                auto path = std::filesystem::path(root) / libraryName / v / (libraryName + ".mtcLib");
                if (std::filesystem::exists(path)) {
                    return std::filesystem::canonical(path).string();
                }
            }
            return std::nullopt;
        };

        if (auto hit = tryVersion(version)) return hit;

        std::string stripped = version;
        while (!stripped.empty() && (stripped.front() == '^' || stripped.front() == '~' ||
                                       stripped.front() == '=' || stripped.front() == '>' ||
                                       stripped.front() == ' ')) {
            stripped.erase(0, 1);
        }
        if (stripped != version && !stripped.empty()) {
            if (auto hit = tryVersion(stripped)) return hit;
        }

        // If the version is a range and lockfile pinning didn't kick in,
        // accept the highest available version under the package dir. This
        // matches the spirit of `^X.Y.Z` while keeping the resolver free of
        // SemVer logic (which lives in packagemanager). Exact requests that
        // had no prefix stripped do NOT reach here — they already returned.
        if (stripped != version)
        {
            return resolveInRegistryHighest(libraryName);
        }
        return std::nullopt;
    }

    std::optional<std::string> MtcPathResolver::resolveInRegistryHighest(
        const std::string& libraryName) const
    {
        std::vector<std::filesystem::path> candidates;
        for (const auto& root : registryRoots) {
            auto pkgDir = std::filesystem::path(root) / libraryName;
            std::error_code ec;
            if (!std::filesystem::exists(pkgDir, ec) || !std::filesystem::is_directory(pkgDir, ec)) {
                continue;
            }
            for (const auto& entry : std::filesystem::directory_iterator(pkgDir, ec)) {
                if (!entry.is_directory(ec)) continue;
                auto candidate = entry.path() / (libraryName + ".mtcLib");
                if (std::filesystem::exists(candidate, ec)) {
                    candidates.push_back(candidate);
                }
            }
        }

        if (candidates.empty()) return std::nullopt;

        // Sort by the version segment (directory name), comparing numeric
        // dotted components so 1.9.0 < 1.10.0 (lexicographic order would
        // pick 1.9.0 as larger). Inlined rather than pulling in
        // packagemanager::SemVer so the resolver stays free of that dep —
        // the comparator below only needs the dotted-int prefix and falls
        // back to lex compare on any trailing non-numeric piece (pre-release,
        // build metadata). On exact-version tie the latest mtime wins.
        std::sort(candidates.begin(), candidates.end(),
            [](const std::filesystem::path& a, const std::filesystem::path& b) {
                const std::string verA = a.parent_path().filename().string();
                const std::string verB = b.parent_path().filename().string();
                if (compareDottedVersions(verA, verB) < 0) return true;
                if (compareDottedVersions(verA, verB) > 0) return false;
                std::error_code ec;
                return std::filesystem::last_write_time(a, ec) <
                       std::filesystem::last_write_time(b, ec);
            });
        return std::filesystem::canonical(candidates.back()).string();
    }

    int MtcPathResolver::compareDottedVersions(const std::string& a, const std::string& b)
    {
        // Split each version into numeric components plus a trailing tail.
        // "1.10.0-rc.1" → components=[1, 10, 0], tail="-rc.1".
        auto parse = [](const std::string& s, std::vector<unsigned long long>& comps,
                        std::string& tail) {
            size_t i = 0;
            while (i < s.size()) {
                if (s[i] < '0' || s[i] > '9') break;
                unsigned long long n = 0;
                while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
                    // Saturate on overflow rather than wrap — keeps ordering sane.
                    if (n > (~0ULL - 9) / 10) { n = ~0ULL; ++i; continue; }
                    n = n * 10 + static_cast<unsigned long long>(s[i] - '0');
                    ++i;
                }
                comps.push_back(n);
                if (i < s.size() && s[i] == '.') { ++i; continue; }
                break;
            }
            tail = s.substr(i);
        };

        std::vector<unsigned long long> compsA, compsB;
        std::string tailA, tailB;
        parse(a, compsA, tailA);
        parse(b, compsB, tailB);

        // Compare component by component; missing trailing components are 0.
        size_t n = std::max(compsA.size(), compsB.size());
        for (size_t i = 0; i < n; ++i) {
            unsigned long long va = i < compsA.size() ? compsA[i] : 0ULL;
            unsigned long long vb = i < compsB.size() ? compsB[i] : 0ULL;
            if (va < vb) return -1;
            if (va > vb) return  1;
        }

        // Numeric prefix equal — fall back to lex on the tail. Note: this
        // does NOT implement SemVer pre-release precedence (1.0.0-alpha <
        // 1.0.0). For registry resolution that's acceptable; users wanting
        // strict pre-release ordering should pin via lockfile.
        if (tailA < tailB) return -1;
        if (tailA > tailB) return  1;
        return 0;
    }
}
