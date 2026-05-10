#include "PluginLoader.hpp"
#include "PluginHost.hpp"
#include "PluginHostApi.h"

#include "../environment/Environment.hpp"
#include "../environment/registry/NativeRegistry.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"
#include "../errors/RuntimeException.hpp"

#include <filesystem>

namespace plugin
{
    namespace
    {
        constexpr const char* kPluginEntryPoint = "mtype_plugin_register";

        using PluginRegisterFn = int (*)(uint32_t,
                                         const ::MTypePluginHost*,
                                         ::MTypeContext*);

        std::string normalizePath(const std::string& p)
        {
            std::error_code ec;
            auto canon = std::filesystem::weakly_canonical(std::filesystem::path(p), ec);
            if (ec) return p;
            return canon.string();
        }
    }

    PluginLoader& PluginLoader::instance()
    {
        static PluginLoader inst;
        return inst;
    }

    bool PluginLoader::isLoaded(const std::string& path) const
    {
        std::lock_guard<std::mutex> lk(mtx_);
        return loaded_.find(normalizePath(path)) != loaded_.end();
    }

    void PluginLoader::load(const std::string& path,
                             std::shared_ptr<::environment::Environment> env,
                             std::shared_ptr<::vm::runtime::VirtualMachine> vm)
    {
        if (path.empty())
        {
            throw ::errors::RuntimeException("PluginError: path cannot be empty");
        }
        if (!std::filesystem::exists(path))
        {
            throw ::errors::RuntimeException("PluginError: file not found: " + path);
        }

        std::string key = normalizePath(path);

        std::lock_guard<std::mutex> lk(mtx_);
        if (loaded_.find(key) != loaded_.end())
        {
            throw ::errors::RuntimeException("PluginError: already loaded: " + key);
        }

        std::string osErr;
        void* osHandle = osLoad(path, osErr);
        if (!osHandle)
        {
            throw ::errors::RuntimeException("PluginError: failed to load '" + path + "': " + osErr);
        }

        auto registerFn = reinterpret_cast<PluginRegisterFn>(osSym(osHandle, kPluginEntryPoint));
        if (!registerFn)
        {
            osClose(osHandle);
            throw ::errors::RuntimeException(
                "PluginError: missing entry point '" + std::string(kPluginEntryPoint) +
                "' in '" + path + "'");
        }

        auto handle = std::make_unique<PluginHandle>();
        handle->path = key;
        handle->osHandle = osHandle;

        ::MTypeContext regCtx;
        regCtx.env = env;
        regCtx.vm = vm;
        regCtx.registeringPlugin = handle.get();

        int rc = 0;
        try
        {
            rc = registerFn(MTYPE_PLUGIN_ABI_VERSION, getHostVTable(), &regCtx);
        }
        catch (const std::exception& e)
        {
            /* Plugin shouldn't throw, but if it does, undo registration. */
            for (const auto& name : handle->registeredNames)
            {
                env->getNativeRegistry()->unregisterNativeFunction(name);
            }
            osClose(osHandle);
            throw ::errors::RuntimeException(
                "PluginError: '" + path + "' threw during register: " + e.what());
        }

        if (regCtx.errorPending)
        {
            std::string type = std::move(regCtx.errorType);
            std::string msg  = std::move(regCtx.errorMessage);
            for (const auto& name : handle->registeredNames)
            {
                env->getNativeRegistry()->unregisterNativeFunction(name);
            }
            osClose(osHandle);
            throw ::errors::RuntimeException(type + ": " + msg);
        }

        if (rc != 0)
        {
            for (const auto& name : handle->registeredNames)
            {
                env->getNativeRegistry()->unregisterNativeFunction(name);
            }
            osClose(osHandle);
            throw ::errors::RuntimeException(
                "PluginError: '" + path + "' register returned " + std::to_string(rc));
        }

        loaded_.emplace(key, std::move(handle));
    }

    void PluginLoader::unload(const std::string& path,
                               std::shared_ptr<::environment::Environment> env,
                               std::shared_ptr<::vm::runtime::VirtualMachine> vm)
    {
        std::string key = normalizePath(path);

        std::unique_ptr<PluginHandle> handle;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            auto it = loaded_.find(key);
            if (it == loaded_.end())
            {
                throw ::errors::RuntimeException("PluginError: not loaded: " + key);
            }
            handle = std::move(it->second);
            loaded_.erase(it);
        }

        auto registry = env ? env->getNativeRegistry() : nullptr;
        if (registry)
        {
            for (const auto& name : handle->registeredNames)
            {
                registry->unregisterNativeFunction(name);
            }
        }

        invalidateNativeCaches(vm);

        /* Bindings drop here, so the trampoline's userData becomes inaccessible
         * BEFORE we close the OS handle that holds the plugin's fn pointers. */
        handle->bindings.clear();
        osClose(handle->osHandle);
    }

    void PluginLoader::unloadAll()
    {
        std::lock_guard<std::mutex> lk(mtx_);
        for (auto& [path, handle] : loaded_)
        {
            handle->bindings.clear();
            if (handle->osHandle) osClose(handle->osHandle);
        }
        loaded_.clear();
    }

    void PluginLoader::invalidateNativeCaches(const std::shared_ptr<::vm::runtime::VirtualMachine>& vm)
    {
        if (!vm) return;
        for (const auto* prog : vm->getLoadedPrograms())
        {
            if (prog) prog->clearNativeCacheSlots();
        }
    }
}
