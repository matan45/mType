#include "LibraryNatives.hpp"
#include "../../environment/registry/NativeRegistry.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../value/InternedString.hpp"
#include "../../value/ValueShim.hpp"
#include "../../vm/runtime/VirtualMachine.hpp"
#include <filesystem>

namespace project::mtclib
{
    void LibraryNatives::registerAll(std::shared_ptr<environment::Environment> env, std::shared_ptr<TransitiveDependencyLoader> loader)
    {
        auto nativeRegistry = env->getNativeRegistry();

        // Pass the loader as userData. registerAll is only called by ScriptInterpreter, 
        // which keeps 'loader' alive for the duration of the script.
        nativeRegistry->registerNativeFunction("loadLibrary", environment::registry::NativeFunction{
            loader.get(),
            [](void* u, NativeContext& c, std::span<const value::Value> a) { return nativeLoadLibrary(u, c, a); }
        });
        nativeRegistry->registerNativeFunction("unloadLibrary", environment::registry::NativeFunction{
            loader.get(),
            [](void* u, NativeContext& c, std::span<const value::Value> a) { return nativeUnloadLibrary(u, c, a); }
        });
    }

    void LibraryNatives::cleanup()
    {
    }

    value::Value LibraryNatives::nativeLoadLibrary(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        if (args.size() != 1) {
            throw errors::RuntimeException(
                "loadLibrary expects 1 argument, got " + std::to_string(args.size()));
        }

        std::string path = extractString(args[0], "loadLibrary", "path");
        validateLibraryPath(path);

        auto* loader = static_cast<TransitiveDependencyLoader*>(userData);
        if (!ctx.vm || !loader) {
            throw errors::RuntimeException(
                "loadLibrary: runtime not initialized");
        }

        try {
            loader->loadLibraryWithDependencies(path, *ctx.vm, ctx.env);
        }
        catch (const std::exception& e) {
            throw errors::RuntimeException(
                "loadLibrary failed: " + std::string(e.what()));
        }

        return std::monostate{};
    }

    value::Value LibraryNatives::nativeUnloadLibrary(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        if (args.size() != 1) {
            throw errors::RuntimeException(
                "unloadLibrary expects 1 argument, got " + std::to_string(args.size()));
        }

        std::string name = extractString(args[0], "unloadLibrary", "name");

        if (name.empty()) {
            throw errors::RuntimeException("unloadLibrary: library name cannot be empty");
        }

        auto* loader = static_cast<TransitiveDependencyLoader*>(userData);
        if (!ctx.vm || !loader) {
            throw errors::RuntimeException("unloadLibrary: runtime not initialized");
        }

        if (!ctx.env->isLibraryLoaded(name)) {
            throw errors::RuntimeException(
                "unloadLibrary: library '" + name + "' is not loaded");
        }

        try {
            loader->unloadLibrary(name, *ctx.vm, ctx.env);
        }
        catch (const std::exception& e) {
            throw errors::RuntimeException(
                "unloadLibrary failed: " + std::string(e.what()));
        }

        return std::monostate{};
    }

    void LibraryNatives::validateLibraryPath(const std::string& path)
    {
        if (path.empty()) {
            throw errors::RuntimeException("loadLibrary: path cannot be empty");
        }

        if (!std::filesystem::exists(path)) {
            throw errors::RuntimeException("loadLibrary: file not found: " + path);
        }

        if (!std::filesystem::is_regular_file(path)) {
            throw errors::RuntimeException("loadLibrary: not a regular file: " + path);
        }

        // Validate .mtcLib extension
        std::filesystem::path fsPath(path);
        if (fsPath.extension() != ".mtcLib") {
            throw errors::RuntimeException(
                "loadLibrary: file must have .mtcLib extension: " + path);
        }
    }

    std::string LibraryNatives::extractString(
        const value::Value& arg, const std::string& funcName, const std::string& paramName)
    {
        // MYT-317: SSO-aware. Folds all three string forms into one branch.
        if (value::isAnyString(arg)) {
            return std::string(value::asStringView(arg));
        }
        throw errors::RuntimeException(
            funcName + ": " + paramName + " must be a string");
    }
}



