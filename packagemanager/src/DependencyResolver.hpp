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
        std::string source;  // e.g. "github:user/repo", empty = local registry
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
        explicit DependencyResolver(PackageRegistry& registry);

        std::unordered_map<std::string, ResolvedPackage> resolve(
            const std::vector<PackageDependency>& directDeps);

    private:
        PackageRegistry& registry;

        void resolveRecursive(
            const std::string& name,
            const std::string& versionRange,
            std::unordered_map<std::string, ResolvedPackage>& resolved,
            std::vector<std::string>& resolveStack);
    };
}
