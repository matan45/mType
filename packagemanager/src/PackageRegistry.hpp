#pragma once
#include "SemVer.hpp"
#include "PackageManifest.hpp"
#include <string>
#include <vector>

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

        static std::string getDefaultRegistryRoot();

    private:
        std::string registryRoot;
    };
}
