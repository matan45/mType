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

    std::vector<MtcLibProgram> LibraryLinker::linkDependencies(const ProjectConfig& config)
    {
        loadedLibraries.clear();

        // Load each declared dependency and its transitive dependencies
        for (const auto& dep : config.dependencies.packages) {
            loadLibraryRecursive(dep.name, dep.versionRange);
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

        // Resolve path
        auto libPath = versionConstraint.empty()
            ? pathResolver.resolve(libraryName)
            : pathResolver.resolve(libraryName, versionConstraint);

        if (!libPath) {
            std::string searchedPaths;
            for (const auto& sp : pathResolver.getSearchPaths()) {
                if (!searchedPaths.empty()) searchedPaths += ", ";
                searchedPaths += "'" + sp + "'";
            }
            throw std::runtime_error(
                "Library '" + libraryName + "' not found. Searched: " + searchedPaths);
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
}
