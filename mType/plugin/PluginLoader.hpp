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

        /* MYT-325 follow-up: register an additional directory to consult when
         * a path passed to load() does not exist as-is. Used by the launcher
         * to add exeDir so that user code calling
         * `__plugin_load("mt_modules/.../foo.dll")` still resolves after the
         * exe has been moved out of the project root (CWD is `build/` rather
         * than the project root that the path string was authored against).
         * Idempotent — duplicate adds are silently dropped. */
        void addSearchPath(const std::string& dir);

    private:
        PluginLoader() = default;
        PluginLoader(const PluginLoader&) = delete;
        PluginLoader& operator=(const PluginLoader&) = delete;

        /* Platform-specific. Defined in WinPluginLoader.cpp / PosixPluginLoader.cpp.
         * On failure return nullptr and fill outErr with a human-readable message. */
        static void* osLoad(const std::string& path, std::string& outErr);
        static void* osSym(void* handle, const char* symbol);
        static void  osClose(void* handle);

        /* Returns the first existing-on-disk resolution of `path`: either the
         * path verbatim or `<searchRoot>/<path>` for some registered root.
         * Empty string if none found. Takes the lock internally. */
        std::string resolveAgainstSearchPaths(const std::string& path) const;

        /* Invalidates cached NativeDelegate slots in every loaded BytecodeProgram
         * so a still-warm IC doesn't jump into the freed plugin image. */
        static void invalidateNativeCaches(const std::shared_ptr<::vm::runtime::VirtualMachine>& vm);

        /* MYT-316: defensive sweep for inlined-function-caller invalidation
         * on plugin unload. Plugin functions are native and not currently
         * inlineable (CALLEE_NATIVE rejection in InlineAnalysis), so this is
         * a no-op for today's plugins — but a future plugin API that ships
         * mType functions would need this hook, and the JIT'd caller
         * eviction path already exists, so wire it in now. */
        static void invalidateInlinedCallers(const std::shared_ptr<::vm::runtime::VirtualMachine>& vm,
                                              const std::vector<std::string>& names);

        mutable std::mutex mtx_;
        std::unordered_map<std::string, std::unique_ptr<PluginHandle>> loaded_;
        std::vector<std::string> searchPaths_;
    };
}
