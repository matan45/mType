#include "LibraryLinker.hpp"
#include "MtcLibSerializer.hpp"
#include <fstream>
#include <stdexcept>

namespace project::mtclib
{
    LibraryLinker::LibraryLinker(const std::string& projectRoot)
        : pathResolver(projectRoot)
    {
    }

    void LibraryLinker::addSearchPath(const std::string& path)
    {
        pathResolver.addSearchPath(path);
    }

    void LibraryLinker::setLockfileVersions(
        const std::unordered_map<std::string, std::string>& versions)
    {
        lockedVersions = versions;
    }

    std::vector<MtcLibProgram> LibraryLinker::linkDependencies(const ProjectConfig& config)
    {
        loadedLibraries.clear();

        // Load each declared dependency and its transitive dependencies.
        // Lockfile-pinned versions override the declared range so the resolver
        // points at the exact build the user has on disk.
        for (const auto& dep : config.dependencies.packages) {
            auto lockedIt = lockedVersions.find(dep.name);
            const std::string& version = (lockedIt != lockedVersions.end())
                ? lockedIt->second
                : dep.versionRange;
            loadLibraryRecursive(dep.name, version);
        }

        if (loadedLibraries.empty()) {
            return {};
        }

        // Resolve dependency ordering
        auto resolution = dependencyResolver.resolve(loadedLibraries);
        if (!resolution.success) {
            throw std::runtime_error("Library dependency resolution failed: " + resolution.errorMessage);
        }

        // Return libraries in topological load order
        std::vector<MtcLibProgram> result;
        for (const auto& name : resolution.loadOrder) {
            auto it = loadedLibraries.find(name);
            if (it != loadedLibraries.end()) {
                result.push_back(std::move(it->second));
            }
        }

        return result;
    }

    void LibraryLinker::loadLibraryRecursive(
        const std::string& libraryName, const std::string& versionConstraint)
    {
        // Skip if already loaded (deduplication)
        if (loadedLibraries.count(libraryName)) {
            return;
        }

        // Resolve path. Lockfile pinning is applied at the top-level call site
        // in linkDependencies; transitive deps still flow through the declared
        // versionConstraint as encoded in the .mtcLib metadata.
        auto lockedIt = lockedVersions.find(libraryName);
        std::string effectiveVersion = (lockedIt != lockedVersions.end())
            ? lockedIt->second
            : versionConstraint;

        auto libPath = effectiveVersion.empty()
            ? pathResolver.resolve(libraryName)
            : pathResolver.resolve(libraryName, effectiveVersion);

        if (!libPath) {
            throwLibraryNotFound(libraryName);
        }

        // Deserialize
        std::ifstream inFile(*libPath, std::ios::binary);
        if (!inFile) {
            throw std::runtime_error("Could not open library file: " + *libPath);
        }

        MtcLibProgram lib = MtcLibSerializer::deserialize(inFile);
        inFile.close();

        // Store before resolving transitive deps (prevents cycles)
        loadedLibraries[libraryName] = std::move(lib);

        // Recursively load transitive dependencies
        for (const auto& dep : loadedLibraries[libraryName].dependencies) {
            loadLibraryRecursive(dep.name, dep.versionConstraint);
        }
    }

    void LibraryLinker::throwLibraryNotFound(const std::string& libraryName) const
    {
        std::string searchedPaths;
        for (const auto& sp : pathResolver.getSearchPaths()) {
            if (!searchedPaths.empty()) searchedPaths += ", ";
            searchedPaths += "'" + sp + "'";
        }
        for (const auto& rr : pathResolver.getRegistryRoots()) {
            if (!searchedPaths.empty()) searchedPaths += ", ";
            searchedPaths += "'" + rr + "/<version>'";
        }

        // ProjectBuilder partitions <Package> deps and only routes ones WITHOUT
        // mt_modules source through here, so reaching this error means the dep
        // is genuinely missing — neither source nor bytecode is on disk.
        throw std::runtime_error(
            "Library '" + libraryName + "' not found.\n"
            "  Declared as a <Package> dependency but no source (mt_modules/@" +
            libraryName + "/) or compiled .mtcLib is on disk.\n"
            "  Run `mtpm install` to fetch the package from its <Source>, or\n"
            "  place a pre-built " + libraryName + ".mtcLib on a search path.\n"
            "  Searched: " + searchedPaths);
    }
}
