#pragma once
#include "../../project/mtclib/MtcLibFormat.hpp"
#include <cstddef>
#include "../../environment/Environment.hpp"
#include "VirtualMachine.hpp"
#include <memory>
#include <string>
#include <vector>

namespace vm::runtime
{
    /**
     * Loads .mtcLib files into the VM at runtime.
     * Validates the library, registers classes/interfaces/functions
     * into the environment, and adds the BytecodeProgram to the VM.
     */
    class LibraryLoader
    {
    public:
        // Load a .mtcLib from file path
        size_t loadLibrary(const std::string& path,
                          VirtualMachine& vm,
                          std::shared_ptr<environment::Environment> env);

        // Load from pre-parsed MtcLibProgram
        size_t loadLibrary(project::mtclib::MtcLibProgram& lib,
                          VirtualMachine& vm,
                          std::shared_ptr<environment::Environment> env);

    private:
        // Owns the loaded library programs to keep BytecodePrograms alive
        std::vector<std::unique_ptr<project::mtclib::MtcLibProgram>> loadedLibraries;

        void registerSymbolsFromLibrary(
            const vm::bytecode::BytecodeProgram& program,
            std::shared_ptr<environment::Environment> env);
    };
}
