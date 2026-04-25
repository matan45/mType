#pragma once
#include "TransitiveDependencyLoader.hpp"
#include "../../value/ValueType.hpp"
#include "../../environment/Environment.hpp"
#include "../../environment/registry/NativeRegistry.hpp"
#include <memory>
#include <vector>
#include <span>

namespace vm::runtime { class VirtualMachine; }

namespace project::mtclib
{
    /**
     * Native functions for runtime library loading from mType code.
     */
    class LibraryNatives
    {
    public:
        // Register all library native functions in the environment
        static void registerAll(std::shared_ptr<environment::Environment> env, std::shared_ptr<TransitiveDependencyLoader> loader);

        // Cleanup static resources
        static void cleanup();

    private:
        using NativeContext = environment::NativeContext;

        // Native function: loadLibrary(path: string): void
        static value::Value nativeLoadLibrary(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // Native function: unloadLibrary(name: string): void
        static value::Value nativeUnloadLibrary(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // Validate library path for security and correctness
        static void validateLibraryPath(const std::string& path);

        // Extract string from Value argument
        static std::string extractString(
            const value::Value& arg, const std::string& funcName, const std::string& paramName);
    };
}
