#pragma once
#include "MtcLibFormat.hpp"
#include "../../environment/Environment.hpp"
#include <memory>
#include <string>

namespace project::mtclib
{
    /**
     * Creates compile-time type stubs from library metadata.
     * Registers ClassDefinition, InterfaceDefinition, and FunctionDefinition
     * objects into the environment for type checking, without loading bytecode.
     */
    class LibrarySymbolProvider
    {
    public:
        // Register all public symbols from a library into the environment
        static void registerLibrarySymbols(
            const MtcLibProgram& library,
            std::shared_ptr<environment::Environment> environment);

    private:
        static void registerClassStubs(
            const vm::bytecode::BytecodeProgram& program,
            std::shared_ptr<environment::Environment> environment);

        static void registerInterfaceStubs(
            const vm::bytecode::BytecodeProgram& program,
            std::shared_ptr<environment::Environment> environment);

        static void registerFunctionStubs(
            const vm::bytecode::BytecodeProgram& program,
            std::shared_ptr<environment::Environment> environment);
    };
}
