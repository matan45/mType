#pragma once
#include "TransitiveDependencyLoader.hpp"
#include "../../value/ValueType.hpp"
#include "../../environment/Environment.hpp"
#include <memory>
#include <vector>

namespace vm::runtime { class VirtualMachine; }

namespace project::mtclib
{
    /**
     * Native functions for runtime library loading from mType code.
     * Follows the ReflectionNatives pattern with static state for VM/loader access.
     *
     * Usage in mType:
     *   loadLibrary("path/to/MyLib.mtcLib");
     */
    class LibraryNatives
    {
    public:
        // Register all library native functions in the environment
        static void registerAll(std::shared_ptr<environment::Environment> env);

        // Set the current VM (called during ScriptInterpreter initialization)
        static void setVM(std::shared_ptr<vm::runtime::VirtualMachine> vm);

        // Set the transitive dependency loader (called during ScriptInterpreter initialization)
        static void setLoader(std::shared_ptr<TransitiveDependencyLoader> loader);

        // Cleanup static resources (call before program exit)
        static void cleanup();

    private:
        // Native function: loadLibrary(path: string): void
        static value::Value __lib_loadLibrary(const std::vector<value::Value>& args);

        // Validate library path for security and correctness
        static void validateLibraryPath(const std::string& path);

        // Extract string from Value argument
        static std::string extractString(
            const value::Value& arg, const std::string& funcName, const std::string& paramName);

        static std::shared_ptr<environment::Environment> currentEnvironment;
        static std::shared_ptr<vm::runtime::VirtualMachine> currentVM;
        static std::shared_ptr<TransitiveDependencyLoader> currentLoader;
    };
}
