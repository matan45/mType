#include "DependencyResolver.hpp"
#include <stdexcept>
#include <algorithm>

namespace packagemanager
{
    DependencyResolver::DependencyResolver(const PackageRegistry& registry)
        : registry(registry)
    {
    }

    std::unordered_map<std::string, ResolvedPackage> DependencyResolver::resolve(
        const std::vector<PackageDependency>& directDeps)
    {
        std::unordered_map<std::string, ResolvedPackage> resolved;
        std::vector<std::string> resolveStack;

        for (const auto& dep : directDeps)
        {
            resolveRecursive(dep.name, dep.versionRange, resolved, resolveStack);
        }

        return resolved;
    }

    void DependencyResolver::resolveRecursive(
        const std::string& name,
        const std::string& versionRange,
        std::unordered_map<std::string, ResolvedPackage>& resolved,
        std::vector<std::string>& resolveStack)
    {
        // Circular dependency check
        for (const auto& item : resolveStack)
        {
            if (item == name)
            {
                std::string chain;
                for (const auto& s : resolveStack) chain += s + " -> ";
                chain += name;
                throw std::runtime_error("Circular dependency detected: " + chain);
            }
        }

        // Parse the version range
        SemVerRange range = SemVerRange::parse(versionRange);

        // If already resolved, check compatibility
        auto it = resolved.find(name);
        if (it != resolved.end())
        {
            SemVer existingVer = SemVer::parse(it->second.version);
            if (range.satisfiedBy(existingVer))
            {
                return; // Already resolved with a compatible version
            }
            throw std::runtime_error(
                "Version conflict for package '" + name + "': " +
                "already resolved to " + it->second.version +
                " but " + versionRange + " is also required");
        }

        // Find available versions
        auto versions = registry.getAvailableVersions(name);
        if (versions.empty())
        {
            throw std::runtime_error("Package not found in registry: " + name);
        }

        // Find best match
        SemVer bestVersion = findBestMatch(versions, range);

        // Get manifest for transitive dependencies
        PackageManifest manifest = registry.getManifest(name, bestVersion.toString());

        // Record resolution
        ResolvedPackage pkg;
        pkg.name = name;
        pkg.version = bestVersion.toString();
        pkg.registryPath = registry.getPackagePath(name, bestVersion.toString());
        pkg.dependencies = manifest.dependencies;

        resolved[name] = pkg;

        // Resolve transitive dependencies
        resolveStack.push_back(name);
        for (const auto& [depName, depRange] : manifest.dependencies)
        {
            resolveRecursive(depName, depRange, resolved, resolveStack);
        }
        resolveStack.pop_back();
    }
}
