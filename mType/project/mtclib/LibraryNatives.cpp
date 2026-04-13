#include "LibraryNatives.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../value/InternedString.hpp"
#include "../../vm/runtime/VirtualMachine.hpp"
#include <filesystem>

namespace project::mtclib
{
    // Static member initialization
    std::shared_ptr<environment::Environment> LibraryNatives::currentEnvironment = nullptr;
    std::shared_ptr<vm::runtime::VirtualMachine> LibraryNatives::currentVM = nullptr;
    std::shared_ptr<TransitiveDependencyLoader> LibraryNatives::currentLoader = nullptr;

    void LibraryNatives::setVM(std::shared_ptr<vm::runtime::VirtualMachine> vm)
    {
        currentVM = std::move(vm);
    }

    void LibraryNatives::setLoader(std::shared_ptr<TransitiveDependencyLoader> loader)
    {
        currentLoader = std::move(loader);
    }

    void LibraryNatives::registerAll(std::shared_ptr<environment::Environment> env)
    {
        currentEnvironment = env;
        auto nativeRegistry = env->getNativeRegistry();

        nativeRegistry->registerNativeFunction("loadLibrary", __lib_loadLibrary);
    }

    void LibraryNatives::cleanup()
    {
        currentEnvironment = nullptr;
        currentVM = nullptr;
        currentLoader = nullptr;
    }

    value::Value LibraryNatives::__lib_loadLibrary(const std::vector<value::Value>& args)
    {
        if (args.size() != 1) {
            throw errors::RuntimeException(
                "loadLibrary expects 1 argument, got " + std::to_string(args.size()));
        }

        std::string path = extractString(args[0], "loadLibrary", "path");
        validateLibraryPath(path);

        if (!currentVM || !currentLoader || !currentEnvironment) {
            throw errors::RuntimeException(
                "loadLibrary: runtime not initialized");
        }

        try {
            currentLoader->loadLibraryWithDependencies(path, *currentVM, currentEnvironment);
        }
        catch (const std::exception& e) {
            throw errors::RuntimeException(
                "loadLibrary failed: " + std::string(e.what()));
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
        if (std::holds_alternative<std::string>(arg)) {
            return std::get<std::string>(arg);
        }
        if (std::holds_alternative<value::InternedString>(arg)) {
            return std::get<value::InternedString>(arg).getString();
        }
        throw errors::RuntimeException(
            funcName + ": " + paramName + " must be a string");
    }
}
