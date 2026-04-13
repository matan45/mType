#include "TransitiveDependencyLoader.hpp"
#include "MtcLibSerializer.hpp"
#include <fstream>
#include <stdexcept>

namespace project::mtclib
{
    TransitiveDependencyLoader::TransitiveDependencyLoader(const std::string& projectRoot)
        : pathResolver(projectRoot)
    {
    }

    void TransitiveDependencyLoader::addSearchPath(const std::string& path)
    {
        pathResolver.addSearchPath(path);
    }

    void TransitiveDependencyLoader::loadLibraryWithDependencies(
        const std::string& path,
        vm::runtime::VirtualMachine& vm,
        std::shared_ptr<environment::Environment> env)
    {
        // Deserialize the root library from the given path
        std::ifstream inFile(path, std::ios::binary);
        if (!inFile) {
            throw std::runtime_error("Could not open library file: " + path);
        }

        MtcLibProgram lib = MtcLibSerializer::deserialize(inFile);
        inFile.close();

        const std::string& libName = lib.metadata.name;
        if (env->isLibraryLoaded(libName)) {
            return;
        }

        // Collect this library and all transitive dependencies
        std::unordered_map<std::string, MtcLibProgram> collected;
        collected[libName] = std::move(lib);

        for (const auto& dep : collected[libName].dependencies) {
            collectDependencies(dep.name, dep.versionConstraint, collected, env);
        }

        // Load everything in topological order
        loadInOrder(collected, vm, env);
    }

    void TransitiveDependencyLoader::loadLibrariesWithDependencies(
        const std::vector<std::string>& paths,
        vm::runtime::VirtualMachine& vm,
        std::shared_ptr<environment::Environment> env)
    {
        std::unordered_map<std::string, MtcLibProgram> collected;

        // Deserialize each root library and collect its transitive dependencies
        for (const auto& path : paths) {
            std::ifstream inFile(path, std::ios::binary);
            if (!inFile) {
                throw std::runtime_error("Could not open library file: " + path);
            }

            MtcLibProgram lib = MtcLibSerializer::deserialize(inFile);
            inFile.close();

            const std::string& libName = lib.metadata.name;
            if (env->isLibraryLoaded(libName) || collected.count(libName)) {
                continue;
            }

            collected[libName] = std::move(lib);

            for (const auto& dep : collected[libName].dependencies) {
                collectDependencies(dep.name, dep.versionConstraint, collected, env);
            }
        }

        if (!collected.empty()) {
            loadInOrder(collected, vm, env);
        }
    }

    void TransitiveDependencyLoader::collectDependencies(
        const std::string& libraryName,
        const std::string& versionConstraint,
        std::unordered_map<std::string, MtcLibProgram>& collected,
        std::shared_ptr<environment::Environment> env)
    {
        // Skip if already collected or already loaded in the environment
        if (collected.count(libraryName) || env->isLibraryLoaded(libraryName)) {
            return;
        }

        // Resolve path via MtcPathResolver
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
                "Transitive dependency '" + libraryName + "' not found. Searched: " + searchedPaths);
        }

        // Deserialize
        std::ifstream inFile(*libPath, std::ios::binary);
        if (!inFile) {
            throw std::runtime_error("Could not open library file: " + *libPath);
        }

        MtcLibProgram lib = MtcLibSerializer::deserialize(inFile);
        inFile.close();

        // Store before recursing (prevents cycles, mirrors LibraryLinker pattern)
        collected[libraryName] = std::move(lib);

        // Recurse into transitive dependencies
        for (const auto& dep : collected[libraryName].dependencies) {
            collectDependencies(dep.name, dep.versionConstraint, collected, env);
        }
    }

    void TransitiveDependencyLoader::loadInOrder(
        std::unordered_map<std::string, MtcLibProgram>& collected,
        vm::runtime::VirtualMachine& vm,
        std::shared_ptr<environment::Environment> env)
    {
        // Resolve topological order (detects cycles and version conflicts)
        auto resolution = dependencyResolver.resolve(collected);
        if (!resolution.success) {
            throw std::runtime_error(
                "Library dependency resolution failed: " + resolution.errorMessage);
        }

        // Load each library in topological order.
        // Move programs into ownedPrograms to keep BytecodeProgram pointers alive —
        // the VM stores raw pointers via addLoadedProgram().
        for (const auto& name : resolution.loadOrder) {
            auto it = collected.find(name);
            if (it != collected.end()) {
                auto owned = std::make_unique<MtcLibProgram>(std::move(it->second));
                libraryLoader.loadLibrary(*owned, vm, env);
                ownedPrograms.push_back(std::move(owned));
            }
        }
    }
}
