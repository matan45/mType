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

    void TransitiveDependencyLoader::unloadLibrary(
        const std::string& libraryName,
        vm::runtime::VirtualMachine& vm,
        std::shared_ptr<environment::Environment> env)
    {
        // Find the owned program by name
        MtcLibProgram* targetLib = nullptr;
        size_t targetIndex = 0;
        for (size_t i = 0; i < ownedPrograms.size(); ++i) {
            if (ownedPrograms[i] && ownedPrograms[i]->metadata.name == libraryName) {
                targetLib = ownedPrograms[i].get();
                targetIndex = i;
                break;
            }
        }

        if (!targetLib) {
            throw std::runtime_error(
                "Cannot unload library '" + libraryName + "': not loaded by this loader");
        }

        // Check if any other loaded library depends on this one
        for (const auto& prog : ownedPrograms) {
            if (!prog || prog->metadata.name == libraryName) continue;
            if (!env->isLibraryLoaded(prog->metadata.name)) continue;

            for (const auto& dep : prog->dependencies) {
                if (dep.name == libraryName) {
                    throw std::runtime_error(
                        "Cannot unload library '" + libraryName +
                        "': library '" + prog->metadata.name + "' depends on it");
                }
            }
        }

        // Remove exported symbols from registries
        auto classRegistry = env->getClassRegistry();
        auto functionRegistry = env->getFunctionRegistry();
        auto interfaceRegistry = env->getInterfaceRegistry();

        for (const auto& exp : targetLib->exports) {
            switch (exp.kind) {
                case SymbolKind::CLASS:
                    classRegistry->removeItem(exp.name);
                    break;
                case SymbolKind::INTERFACE:
                    interfaceRegistry->removeInterface(exp.name);
                    break;
                case SymbolKind::FUNCTION:
                    functionRegistry->removeFunction(exp.name);
                    break;
                case SymbolKind::VARIABLE:
                    break;
            }
        }

        // Remove bytecode program from VM using the exact pointer that was registered.
        auto ptrIt = vmProgramPointers.find(libraryName);
        if (ptrIt == vmProgramPointers.end() || !vm.removeLoadedProgram(ptrIt->second)) {
            throw std::runtime_error(
                "Cannot unload library '" + libraryName +
                "': failed to remove bytecode program from VM");
        }
        vmProgramPointers.erase(ptrIt);

        // Unmark from environment
        env->unmarkLibraryLoaded(libraryName);

        // Release ownership (destroys the MtcLibProgram)
        ownedPrograms.erase(ownedPrograms.begin() + static_cast<std::ptrdiff_t>(targetIndex));
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

        // Copy the name before moving — the reference would dangle after std::move
        const std::string libName = lib.metadata.name;
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

            const std::string libName = lib.metadata.name;
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
                // Capture the pointer BEFORE loadLibrary, since that's what addLoadedProgram stores
                const vm::bytecode::BytecodeProgram* progPtr = &owned->bytecodeProgram;
                libraryLoader.loadLibrary(*owned, vm, env);
                vmProgramPointers[owned->metadata.name] = progPtr;
                ownedPrograms.push_back(std::move(owned));
            }
        }
    }
}
