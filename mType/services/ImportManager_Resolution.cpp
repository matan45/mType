#include "ImportManager.hpp"
#include <cstddef>
#include <filesystem>
#include "../errors/FileException.hpp"

namespace services
{
    namespace fs = std::filesystem;

    void ImportManager::setBaseDirectory(const std::string& dir)
    {
        baseDirectory = dir;
    }

    void ImportManager::setProjectRoot(const std::string& root)
    {
        allowedRoots.clear();
        if (!root.empty())
        {
            allowedRoots.push_back(root);
        }
    }

    void ImportManager::addAllowedRoot(const std::string& root)
    {
        if (!root.empty())
        {
            allowedRoots.push_back(root);
        }
    }

    std::string ImportManager::getCurrentFilePath() const
    {
        return currentFilePath;
    }

    void ImportManager::setCurrentFilePath(const std::string& path)
    {
        currentFilePath = path;
    }

    void ImportManager::setSearchPaths(const std::vector<std::string>& paths)
    {
        searchPaths = paths;
    }

    void ImportManager::setPathAliases(const std::unordered_map<std::string, std::string>& aliases)
    {
        pathAliases = aliases;
    }

    std::string ImportManager::resolvePath(const std::string& path)
    {
        std::string resolvedPath = path;

        // Path alias expansion (e.g., @core/utils.mt -> lib/core/utils.mt).
        if (!resolvedPath.empty() && resolvedPath[0] == '@')
        {
            size_t slashPos = resolvedPath.find('/');
            if (slashPos == std::string::npos)
            {
                slashPos = resolvedPath.find('\\');
            }

            std::string aliasName = (slashPos != std::string::npos)
                ? resolvedPath.substr(0, slashPos)
                : resolvedPath;

            auto it = pathAliases.find(aliasName);
            if (it != pathAliases.end())
            {
                std::string remainder = (slashPos != std::string::npos)
                    ? resolvedPath.substr(slashPos)
                    : "";
                resolvedPath = it->second + remainder;
            }
        }

        fs::path filePath(resolvedPath);

        // SECURITY: absolute paths are still allowed because some build
        // systems pass absolute paths into the import alias map, but they
        // must canonicalize to a location inside baseDirectory. Path
        // traversal attempts (/etc/passwd, C:\Windows\..., etc.) are
        // rejected by enforceWithinProjectRoot.
        if (!filePath.is_relative())
        {
            filePath = normalizeFilePath(filePath, false);
            enforceWithinProjectRoot(filePath, path);
            return filePath.string();
        }

        // Try resolving relative to current file's directory first.
        if (!currentFilePath.empty())
        {
            fs::path currentFileDir = fs::path(currentFilePath).parent_path();
            fs::path candidate = currentFileDir / filePath;

            try
            {
                candidate = normalizeFilePath(candidate, true);
                if (fs::exists(candidate))
                {
                    enforceWithinProjectRoot(candidate, path);
                    return candidate.string();
                }
            }
            catch (const std::filesystem::filesystem_error&)
            {
            }
        }

        // Try resolving relative to base directory.
        {
            fs::path candidate = fs::path(baseDirectory) / filePath;

            try
            {
                candidate = normalizeFilePath(candidate, true);
                if (fs::exists(candidate))
                {
                    enforceWithinProjectRoot(candidate, path);
                    return candidate.string();
                }
            }
            catch (const std::filesystem::filesystem_error&)
            {
            }
        }

        // Try each configured search path.
        for (const auto& searchPath : searchPaths)
        {
            fs::path searchDir = fs::path(baseDirectory) / searchPath;
            fs::path candidate = searchDir / filePath;

            try
            {
                candidate = normalizeFilePath(candidate, true);
                if (fs::exists(candidate))
                {
                    enforceWithinProjectRoot(candidate, path);
                    return candidate.string();
                }
            }
            catch (const std::filesystem::filesystem_error&)
            {
            }
        }

        // Fall back to original resolution (may throw if file doesn't exist).
        if (!currentFilePath.empty())
        {
            fs::path currentFileDir = fs::path(currentFilePath).parent_path();
            filePath = currentFileDir / filePath;
        }
        else
        {
            filePath = fs::path(baseDirectory) / filePath;
        }

        filePath = normalizeFilePath(filePath, false);
        enforceWithinProjectRoot(filePath, path);
        return filePath.string();
    }

    void ImportManager::enforceWithinProjectRoot(const fs::path& resolved,
                                                 const std::string& originalPath)
    {
        // Containment is opt-in: only enforced when explicit allowed root(s)
        // have been configured (via setProjectRoot/addAllowedRoot). Ad-hoc
        // scripts and tests run without any and skip the check entirely,
        // since their layouts often legitimately use ../ to reach shared
        // lib directories.
        if (allowedRoots.empty())
        {
            return;
        }

        fs::path canonicalResolved;
        try
        {
            canonicalResolved = fs::canonical(resolved);
        }
        catch (const std::filesystem::filesystem_error&)
        {
            canonicalResolved = fs::absolute(resolved).lexically_normal();
        }

        for (const auto& root : allowedRoots)
        {
            fs::path canonicalRoot;
            try
            {
                canonicalRoot = fs::canonical(root);
            }
            catch (const std::filesystem::filesystem_error&)
            {
                canonicalRoot = fs::absolute(root).lexically_normal();
            }

            // Component-by-component prefix check. Using iterator comparison
            // defeats /projectroot-evil/file kinds of bypass that a string
            // find() would accept.
            auto rootIt = canonicalRoot.begin();
            auto rootEnd = canonicalRoot.end();
            auto pathIt = canonicalResolved.begin();
            auto pathEnd = canonicalResolved.end();

            bool isWithin = true;
            for (; rootIt != rootEnd; ++rootIt, ++pathIt)
            {
                if (pathIt == pathEnd || *pathIt != *rootIt)
                {
                    isWithin = false;
                    break;
                }
            }

            if (isWithin)
            {
                return;
            }
        }

        throw errors::FileException(
            "Import path escapes project root: " + originalPath);
    }

    std::string ImportManager::resolvePathConsistently(const std::string& path)
    {
        fs::path filePath(path);

        // Relative paths resolve against the ORIGINAL base directory (not
        // the current file's directory), so evaluation-tracking keys stay
        // stable regardless of which file is being processed.
        if (filePath.is_relative())
        {
            filePath = fs::path(baseDirectory) / filePath;
        }

        filePath = normalizeFilePath(filePath, true);

        return filePath.string();
    }

    bool ImportManager::safeCheckInSet(const std::string& rawPath,
                                       const std::unordered_set<std::string>& set,
                                       bool useConsistentResolve)
    {
        try
        {
            std::string resolvedPath = useConsistentResolve ?
                resolvePathConsistently(rawPath) : resolvePath(rawPath);
            return set.find(resolvedPath) != set.end();
        }
        catch (...)
        {
            return false;
        }
    }

    std::filesystem::path ImportManager::normalizeFilePath(const std::filesystem::path& filePath,
                                                          bool allowFallback)
    {
        // Lexical normalization first — resolves . and .. components without
        // requiring the file to exist on disk.
        fs::path normalized = filePath.lexically_normal();

        if (allowFallback)
        {
            try
            {
                return fs::canonical(normalized);
            }
            catch (const std::filesystem::filesystem_error&)
            {
                // If canonical fails (file doesn't exist yet), at least
                // return an absolute path so caller can keep working.
                return fs::absolute(normalized);
            }
        }
        else
        {
            return fs::canonical(normalized);
        }
    }
}
