#pragma once
#include "SemVer.hpp"
#include "PackageRegistry.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace packagemanager
{
    struct PackageDependency
    {
        std::string name;
        std::string versionRange;
    };

    struct ResolvedPackage
    {
        std::string name;
        std::string version;
        std::string registryPath;
        std::unordered_map<std::string, std::string> dependencies;
    };

    class DependencyResolver
    {
    public:
        explicit DependencyResolver(const PackageRegistry& registry);

        std::unordered_map<std::string, ResolvedPackage> resolve(
            const std::vector<PackageDependency>& directDeps);

    private:
        const PackageRegistry& registry;

        void resolveRecursive(
            const std::string& name,
            const std::string& versionRange,
            std::unordered_map<std::string, ResolvedPackage>& resolved,
            std::vector<std::string>& resolveStack);
    };
}
