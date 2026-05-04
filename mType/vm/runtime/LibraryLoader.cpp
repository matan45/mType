#include "LibraryLoader.hpp"
#include <cstddef>
#include "../../project/mtclib/MtcLibSerializer.hpp"
#include "../../project/mtclib/LibrarySymbolProvider.hpp"
#include <fstream>
#include <stdexcept>
#include <cstring>

namespace vm::runtime
{
    size_t LibraryLoader::loadLibrary(const std::string& path,
                                      VirtualMachine& vm,
                                      std::shared_ptr<environment::Environment> env)
    {
        std::ifstream inFile(path, std::ios::binary);
        if (!inFile) {
            throw std::runtime_error("Could not open library file: " + path);
        }

        auto lib = std::make_unique<project::mtclib::MtcLibProgram>(
            project::mtclib::MtcLibSerializer::deserialize(inFile));
        inFile.close();

        // Check if already loaded
        const std::string& libName = lib->metadata.name;
        if (env->isLibraryLoaded(libName)) {
            return vm.getLoadedProgramCount();
        }

        // Register symbols
        registerSymbolsFromLibrary(lib->bytecodeProgram, env);

        // Add to VM's loaded programs
        vm.addLoadedProgram(&lib->bytecodeProgram);
        size_t programIndex = vm.getLoadedProgramCount() - 1;

        // Mark as loaded
        env->markLibraryLoaded(libName);

        // Transfer ownership
        loadedLibraries.push_back(std::move(lib));

        return programIndex;
    }

    size_t LibraryLoader::loadLibrary(project::mtclib::MtcLibProgram& lib,
                                      VirtualMachine& vm,
                                      std::shared_ptr<environment::Environment> env)
    {
        const std::string& libName = lib.metadata.name;
        if (env->isLibraryLoaded(libName)) {
            return vm.getLoadedProgramCount();
        }

        // Register symbols
        registerSymbolsFromLibrary(lib.bytecodeProgram, env);

        // Add to VM's loaded programs
        vm.addLoadedProgram(&lib.bytecodeProgram);
        size_t programIndex = vm.getLoadedProgramCount() - 1;

        // Mark as loaded
        env->markLibraryLoaded(libName);

        return programIndex;
    }

    void LibraryLoader::registerSymbolsFromLibrary(
        const vm::bytecode::BytecodeProgram& program,
        std::shared_ptr<environment::Environment> env)
    {
        // Build a minimal MtcLibProgram wrapper for the symbol provider
        project::mtclib::MtcLibProgram wrapper;
        wrapper.bytecodeProgram = program;
        project::mtclib::LibrarySymbolProvider::registerLibrarySymbols(wrapper, env);
    }
}
