#include "PackageRegistry.hpp"
#include "GitSource.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <cstdlib>  // _dupenv_s on Windows, getenv on POSIX

namespace packagemanager
{
    namespace fs = std::filesystem;

    PackageRegistry::PackageRegistry(const std::string& registryRoot)
        : registryRoot(registryRoot)
    {
    }

    std::vector<SemVer> PackageRegistry::getAvailableVersions(const std::string& packageName) const
    {
        std::vector<SemVer> versions;
        fs::path pkgDir = fs::path(registryRoot) / packageName;

        if (!fs::exists(pkgDir) || !fs::is_directory(pkgDir))
        {
            return versions;
        }

        for (const auto& entry : fs::directory_iterator(pkgDir))
        {
            if (entry.is_directory())
            {
                try
                {
                    SemVer ver = SemVer::parse(entry.path().filename().string());
                    versions.push_back(ver);
                }
                catch (const std::exception&)
                {
                    // Skip non-semver directories
                }
            }
        }

        std::sort(versions.begin(), versions.end());
        return versions;
    }

    PackageManifest PackageRegistry::getManifest(const std::string& name, const std::string& version) const
    {
        fs::path manifestPath = fs::path(registryRoot) / name / version / "mtpkg.json";

        if (!fs::exists(manifestPath))
        {
            throw std::runtime_error("Package manifest not found: " + name + "@" + version);
        }

        std::ifstream file(manifestPath);
        std::stringstream buffer;
        buffer << file.rdbuf();

        return PackageManifest::parseFromJson(buffer.str());
    }

    std::string PackageRegistry::getPackagePath(const std::string& name, const std::string& version) const
    {
        fs::path pkgPath = fs::path(registryRoot) / name / version;
        return pkgPath.string();
    }

    bool PackageRegistry::packageExists(const std::string& name, const std::string& version) const
    {
        fs::path pkgPath = fs::path(registryRoot) / name / version / "mtpkg.json";
        return fs::exists(pkgPath);
    }

    static std::string safeGetEnv(const char* name)
    {
#ifdef _WIN32
        char* buf = nullptr;
        size_t len = 0;
        if (_dupenv_s(&buf, &len, name) == 0 && buf != nullptr)
        {
            std::string result(buf);
            free(buf);
            return result;
        }
        return "";
#else
        const char* val = std::getenv(name);
        return val ? val : "";
#endif
    }

    std::string PackageRegistry::getDefaultRegistryRoot()
    {
        std::string envRoot = safeGetEnv("MTYPE_REGISTRY");
        if (!envRoot.empty())
        {
            return envRoot;
        }

        // Default: ~/.mtype/registry/
        std::string home = safeGetEnv("USERPROFILE");
        if (home.empty())
        {
            home = safeGetEnv("HOME");
        }
        if (home.empty())
        {
            throw std::runtime_error("Cannot determine home directory for package registry");
        }

        return (fs::path(home) / ".mtype" / "registry").string();
    }

    void PackageRegistry::registerGitSource(const std::string& packageName, const std::string& source)
    {
        gitSources[packageName] = source;
    }

    std::vector<SemVer> PackageRegistry::getAvailableVersionsWithFetch(const std::string& packageName)
    {
        auto localVersions = getAvailableVersions(packageName);

        auto it = gitSources.find(packageName);
        if (it != gitSources.end())
        {
            std::string cloneUrl = GitSource::toCloneUrl(it->second);
            auto remoteTags = GitSource::listRemoteTags(cloneUrl);

            // Both lists are already sorted — merge with set_union (O(n log n) total)
            std::vector<SemVer> merged;
            merged.reserve(localVersions.size() + remoteTags.size());
            std::set_union(localVersions.begin(), localVersions.end(),
                           remoteTags.begin(), remoteTags.end(),
                           std::back_inserter(merged));
            return merged;
        }

        return localVersions;
    }

    void PackageRegistry::ensureCached(const std::string& packageName, const std::string& version)
    {
        // Already cached locally
        if (packageExists(packageName, version))
        {
            return;
        }

        // Check if we have a git source for this package
        auto it = gitSources.find(packageName);
        if (it == gitSources.end())
        {
            throw std::runtime_error(
                "Package " + packageName + "@" + version +
                " not found in local registry and no git source registered");
        }

        std::string cloneUrl = GitSource::toCloneUrl(it->second);
        GitSource::fetchIntoRegistry(cloneUrl, packageName, version, registryRoot);
    }
}
