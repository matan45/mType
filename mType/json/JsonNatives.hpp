#pragma once
#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"
#include "../environment/registry/NativeRegistry.hpp"
#include <memory>
#include <vector>
#include <span>

namespace json
{
    class JsonNatives
    {
    public:
        static void registerAll(std::shared_ptr<environment::Environment> env);
        static void cleanup();

    private:
        using NativeContext = environment::NativeContext;

        static value::Value __json_serialize(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __json_serializeWithOptions(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __json_deserialize(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __json_deserializeAs(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __json_format(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __json_stringify(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __json_readFile(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __json_writeFile(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        static void validateArgCount(std::span<const value::Value> args,
                                     size_t expected, const std::string& funcName);
        static std::string extractString(const value::Value& arg,
                                         const std::string& funcName);
        static bool extractBool(const value::Value& arg,
                                const std::string& funcName);
    };
}
