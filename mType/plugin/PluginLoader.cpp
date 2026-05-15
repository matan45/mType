#include "PluginLoader.hpp"
#include "PluginHost.hpp"
#include "PluginHostApi.h"

#include "../environment/Environment.hpp"
#include "../environment/registry/NativeRegistry.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"
#include "../value/BridgeArena.hpp"
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

        /* Shared rollback for every load() failure branch: unregister any
         * names the plugin's register fn managed to add. Centralised so the
         * three failure arms can't drift apart. Caller still owns closing
         * the OS handle (osClose is a private static on PluginLoader, not
         * reachable from this anonymous-namespace helper). */
        void rollbackRegistrations(PluginHandle& handle,
                                   const std::shared_ptr<::environment::Environment>& env)
        {
            if (!env) return;
            auto registry = env->getNativeRegistry();
            if (!registry) return;
            for (const auto& name : handle.registeredNames)
            {
                registry->unregisterNativeFunction(name);
            }
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
            rollbackRegistrations(*handle, env);
            osClose(osHandle);
            throw ::errors::RuntimeException(
                "PluginError: '" + path + "' threw during register: " + e.what());
        }

        if (regCtx.errorPending)
        {
            std::string type = std::move(regCtx.errorType);
            std::string msg  = std::move(regCtx.errorMessage);
            rollbackRegistrations(*handle, env);
            osClose(osHandle);
            throw ::errors::RuntimeException(type + ": " + msg);
        }

        if (rc != 0)
        {
            rollbackRegistrations(*handle, env);
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

        /* MYT-316: evict any JIT'd caller that speculatively inlined a function
         * of the same name as one being unbound. Conservative defense — see
         * invalidateInlinedCallers for why this is a no-op for today's
         * native-only plugins, but the hook is in place for future plugin
         * APIs and for the symmetry with invalidateNativeCaches. */
        invalidateInlinedCallers(vm, handle->registeredNames);

        /* Unload ordering (must stay in this order):
         *   1. registry->unregisterNativeFunction (above)  — no new dispatch
         *      can resolve to this plugin's names.
         *   2. invalidateNativeCaches                       — every IC slot
         *      holding a NativeDelegate into this plugin is zeroed.
         *   3. invalidateInlinedCallers                     — every JIT'd
         *      caller that pasted a callee of the same name is evicted
         *      (MYT-316).
         *   4. handle->bindings.clear()                     — trampoline
         *      userData is dropped so no in-flight pointer can be reused.
         *   5. osClose                                       — unmap plugin
         *      code last.
         *
         * Any new cache that stores a NativeDelegate or a raw function pointer
         * into plugin code MUST hook into invalidateNativeCaches; otherwise it
         * will hold a dangling pointer after step 5. */
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
        // MYT-317: drop cached bridge slots. Live Values keep their own
        // bridges; this only purges destructed raw memory blocks so a future
        // bridge kind that ever transitively retained a plugin object cannot
        // sneak it past unload through a recycled slot.
        ::value::BridgeArena::getInstance().reset();
    }

    void PluginLoader::invalidateInlinedCallers(
        const std::shared_ptr<::vm::runtime::VirtualMachine>& vm,
        const std::vector<std::string>& names)
    {
        if (!vm) return;
        // FunctionNameHandle is interned per-BytecodeProgram, so the same
        // function name can have different handle ids across programs.
        // Sweep each program's intern table and invalidate against the
        // VM-level JIT cache — invalidatedInlineCallersOf returns empty for
        // handles that were never registered, so unused programs are
        // effectively no-ops.
        for (const auto* prog : vm->getLoadedPrograms())
        {
            if (!prog) continue;
            for (const auto& name : names)
            {
                const auto h = prog->internFrameName(name);
                vm->invalidateInlinedFunctionCallers(h);
            }
        }
    }
}
