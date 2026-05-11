#include "PluginNatives.hpp"
#include "PluginLoader.hpp"

#include "../environment/registry/NativeRegistry.hpp"
#include "../errors/RuntimeException.hpp"
#include "../value/ValueShim.hpp"
#include "../value/InternedString.hpp"
#include "../vm/runtime/VirtualMachine.hpp"

namespace plugin
{
    void PluginNatives::registerAll(std::shared_ptr<::environment::Environment> env)
    {
        auto registry = env->getNativeRegistry();

        registry->registerNativeFunction("__plugin_load",
            ::environment::registry::NativeDelegate{
                nullptr,
                [](void* u, NativeContext& c, std::span<const ::value::Value> a) {
                    return nativePluginLoad(u, c, a);
                }
            });

        registry->registerNativeFunction("__plugin_unload",
            ::environment::registry::NativeDelegate{
                nullptr,
                [](void* u, NativeContext& c, std::span<const ::value::Value> a) {
                    return nativePluginUnload(u, c, a);
                }
            });
    }

    void PluginNatives::cleanup()
    {
        PluginLoader::instance().unloadAll();
    }

    ::value::Value PluginNatives::nativePluginLoad(void* /*userData*/,
                                                   NativeContext& ctx,
                                                   std::span<const ::value::Value> args)
    {
        if (args.size() != 1)
        {
            throw ::errors::RuntimeException(
                "__plugin_load expects 1 argument (path), got " + std::to_string(args.size()));
        }
        std::string path = extractStringArg(args[0], "__plugin_load");

        if (!ctx.vm)
        {
            throw ::errors::RuntimeException("__plugin_load: VM not initialized");
        }

        PluginLoader::instance().load(path, ctx.env, ctx.vm);
        return ::value::Value();  /* void */
    }

    ::value::Value PluginNatives::nativePluginUnload(void* /*userData*/,
                                                     NativeContext& ctx,
                                                     std::span<const ::value::Value> args)
    {
        if (args.size() != 1)
        {
            throw ::errors::RuntimeException(
                "__plugin_unload expects 1 argument (path), got " + std::to_string(args.size()));
        }
        std::string path = extractStringArg(args[0], "__plugin_unload");

        if (!ctx.vm)
        {
            throw ::errors::RuntimeException("__plugin_unload: VM not initialized");
        }

        PluginLoader::instance().unload(path, ctx.env, ctx.vm);
        return ::value::Value();  /* void */
    }

    std::string PluginNatives::extractStringArg(const ::value::Value& arg,
                                                 const std::string& funcName)
    {
        if (::value::isString(arg))
        {
            return ::value::asString(arg);
        }
        if (::value::isInternedString(arg))
        {
            return ::value::asInternedString(arg).getString();
        }
        throw ::errors::RuntimeException(funcName + ": path must be a string");
    }
}
