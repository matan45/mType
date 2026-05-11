#pragma once
/*
 * Native functions exposed to mType scripts for runtime plugin loading
 * (MYT-289). Two functions:
 *   __plugin_load(path: string): void
 *   __plugin_unload(path: string): void
 */

#include <memory>
#include <span>
#include <string>

#include "../environment/Environment.hpp"
#include "../environment/NativeContext.hpp"
#include "../value/ValueType.hpp"

namespace plugin
{
    class PluginNatives
    {
    public:
        static void registerAll(std::shared_ptr<::environment::Environment> env);

        /* Process-shutdown hook. Closes every still-loaded plugin handle. */
        static void cleanup();

    private:
        using NativeContext = ::environment::NativeContext;

        static ::value::Value nativePluginLoad(void* userData, NativeContext& ctx,
                                                std::span<const ::value::Value> args);
        static ::value::Value nativePluginUnload(void* userData, NativeContext& ctx,
                                                  std::span<const ::value::Value> args);

        static std::string extractStringArg(const ::value::Value& arg,
                                             const std::string& funcName);
    };
}
