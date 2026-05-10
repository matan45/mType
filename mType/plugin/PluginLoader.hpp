#pragma once
/*
 * Native plugin loader (MYT-289). Cross-platform handle abstraction with a
 * Windows implementation in WinPluginLoader.cpp and a POSIX implementation in
 * PosixPluginLoader.cpp. Selected at build time via premake removefiles.
 */

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "PluginHost.hpp"

namespace environment { class Environment; }

namespace plugin
{
    /*
     * One per loaded plugin. Owns the OS handle, the bindings the plugin
     * registered, and the names (used to unregister on unload).
     */
    struct PluginHandle
    {
        std::string path;
        void* osHandle = nullptr;
        std::vector<std::string> registeredNames;
        std::vector<std::unique_ptr<PluginNativeBinding>> bindings;
    };

    class PluginLoader
    {
    public:
        static PluginLoader& instance();

        /* Throws errors::RuntimeException on any failure. */
        void load(const std::string& path,
                  std::shared_ptr<::environment::Environment> env,
                  std::shared_ptr<::vm::runtime::VirtualMachine> vm);

        /* Throws if path is not currently loaded. */
        void unload(const std::string& path,
                    std::shared_ptr<::environment::Environment> env,
                    std::shared_ptr<::vm::runtime::VirtualMachine> vm);

        /* Process-shutdown helper. Closes every loaded handle, swallows errors. */
        void unloadAll();

        bool isLoaded(const std::string& path) const;

    private:
        PluginLoader() = default;
        PluginLoader(const PluginLoader&) = delete;
        PluginLoader& operator=(const PluginLoader&) = delete;

        /* Platform-specific. Defined in WinPluginLoader.cpp / PosixPluginLoader.cpp.
         * On failure return nullptr and fill outErr with a human-readable message. */
        static void* osLoad(const std::string& path, std::string& outErr);
        static void* osSym(void* handle, const char* symbol);
        static void  osClose(void* handle);

        /* Invalidates cached NativeDelegate slots in every loaded BytecodeProgram
         * so a still-warm IC doesn't jump into the freed plugin image. */
        static void invalidateNativeCaches(const std::shared_ptr<::vm::runtime::VirtualMachine>& vm);

        mutable std::mutex mtx_;
        std::unordered_map<std::string, std::unique_ptr<PluginHandle>> loaded_;
    };
}
