#pragma once
#include "MtcLibFormat.hpp"
#include "../../environment/Environment.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>

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

        // Register only selected symbols from a library (for selective import)
        // Register only selected symbols from a library (for selective import)
        // If selectedSymbols is empty, registers all symbols (same as above)
        static void registerLibrarySymbols(
            const MtcLibProgram& library,
            std::shared_ptr<environment::Environment> environment,
            const std::vector<std::string>& selectedSymbols);

        // Register with aliases: import lib {Vector2 as Vec2} from "lib"
        // aliases maps original name -> alias name
        static void registerLibrarySymbols(
            const MtcLibProgram& library,
            std::shared_ptr<environment::Environment> environment,
            const std::vector<std::string>& selectedSymbols,
            const std::unordered_map<std::string, std::string>& aliases);

    private:
        static void registerClassStubs(
            const vm::bytecode::BytecodeProgram& program,
            std::shared_ptr<environment::Environment> environment,
            const std::unordered_set<std::string>& filter,
            const std::unordered_map<std::string, std::string>& aliases);

        static void registerInterfaceStubs(
            const vm::bytecode::BytecodeProgram& program,
            std::shared_ptr<environment::Environment> environment,
            const std::unordered_set<std::string>& filter,
            const std::unordered_map<std::string, std::string>& aliases);

        static void registerFunctionStubs(
            const vm::bytecode::BytecodeProgram& program,
            std::shared_ptr<environment::Environment> environment,
            const std::unordered_set<std::string>& filter,
            const std::unordered_map<std::string, std::string>& aliases);
    };
}
