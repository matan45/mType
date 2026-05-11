#pragma once
/*
 * Internal host side of the mType native plugin C ABI (MYT-289).
 *
 * Defines the concrete types behind the opaque MTypeValue / MTypeContext
 * handles that plugins see, the per-call arena, the universal trampoline
 * registered as a NativeDelegate for every plugin function, and the singleton
 * MTypePluginHost vtable.
 */

#include <deque>
#include <memory>
#include <string>
#include <span>
#include "PluginHostApi.h"
#include "../value/ValueType.hpp"
#include "../environment/NativeContext.hpp"
#include "../environment/registry/NativeDelegate.hpp"

namespace environment { class Environment; }
namespace vm::runtime { class VirtualMachine; }

namespace plugin
{
    struct PluginHandle;          /* defined in PluginLoader.hpp */
    struct PluginNativeBinding;   /* below */
}

/*
 * Concrete definition of the C-ABI opaque MTypeValue handle. Lives at file
 * scope (no namespace) because the C header declares it as `struct MTypeValue`.
 *
 * Defined BEFORE plugin::CallArena so std::deque<MTypeValue> sees a complete
 * type at template instantiation.
 */
struct MTypeValue
{
    ::value::Value v;
};

namespace plugin
{
    /*
     * Per-call arena. Owns every MTypeValue the plugin allocated via makeXxx /
     * arrayGet / objGet during a single call. std::deque so emplace_back never
     * invalidates pointers we already handed to the plugin.
     */
    struct CallArena
    {
        std::deque<::MTypeValue> values;
    };

    /*
     * One per registered plugin function. Owned by PluginHandle::bindings.
     * Used as the userData of the NativeDelegate so the trampoline can recover
     * the plugin's MTypeNativeFn pointer and userData on every call.
     */
    struct PluginNativeBinding
    {
        ::MTypeNativeFn fn;
        void* userData;
        PluginHandle* owner;        /* non-owning */
        std::string name;
    };

    /*
     * Builds and returns the process-singleton vtable. Threadsafe via static
     * initialisation. Pointers in the table are stable for the process lifetime.
     */
    const ::MTypePluginHost* getHostVTable();

    /*
     * Universal NativeDelegate.invoke for plugin functions. Resolves the
     * binding from userData, sets up the per-call arena and MTypeContext,
     * calls the plugin's fn, copies the returned Value out, drains the arena,
     * rethrows any pending plugin error via NativeContext::throwException.
     */
    ::value::Value pluginNativeTrampoline(void* userData,
                                          ::environment::NativeContext& ctx,
                                          std::span<const ::value::Value> args);
}

/*
 * Concrete definition of MTypeContext. References plugin::CallArena so it
 * lives at file scope after the plugin namespace.
 */
struct MTypeContext
{
    std::shared_ptr<::environment::Environment> env;
    std::shared_ptr<::vm::runtime::VirtualMachine> vm;
    plugin::CallArena arena;

    /* Pending error from raiseError, drained by the trampoline after return. */
    bool errorPending = false;
    std::string errorType;
    std::string errorMessage;

    /* Non-null only while a mtype_plugin_register call is in flight. The
     * registerFunction host op pushes a NativeDelegate into this plugin's
     * NativeRegistry under the supplied name. */
    plugin::PluginHandle* registeringPlugin = nullptr;
};
