#pragma once
#include "SemVer.hpp"
#include "PackageManifest.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace packagemanager
{
    class PackageRegistry
    {
    public:
        explicit PackageRegistry(const std::string& registryRoot);

        std::vector<SemVer> getAvailableVersions(const std::string& packageName) const;

        PackageManifest getManifest(const std::string& name, const std::string& version) const;

        std::string getPackagePath(const std::string& name, const std::string& version) const;

        bool packageExists(const std::string& name, const std::string& version) const;

        const std::string& getRegistryRoot() const { return registryRoot; }

        // Register a git source for a package (e.g. "github:user/repo")
        void registerGitSource(const std::string& packageName, const std::string& source);

        // Get available versions, fetching from git if a source is registered
        // and the package isn't in the local cache
        std::vector<SemVer> getAvailableVersionsWithFetch(const std::string& packageName);

        // Ensure a specific version is cached locally (fetches from git if needed)
        void ensureCached(const std::string& packageName, const std::string& version);

        static std::string getDefaultRegistryRoot();

    private:
        std::string registryRoot;
        std::unordered_map<std::string, std::string> gitSources; // name -> source URI
    };
}
