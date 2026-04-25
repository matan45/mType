#include "JsonNatives.hpp"
#include "JsonSerializer.hpp"
#include "JsonDeserializer.hpp"
#include "JsonParser.hpp"
#include "FileWriteNative.hpp"
#include "../services/FileReader.hpp"
#include "../errors/RuntimeException.hpp"
#include "../value/ValueType.hpp"
#include <stdexcept>

namespace json
{
    void JsonNatives::registerAll(std::shared_ptr<environment::Environment> env)
    {
        auto registry = env->getNativeRegistry();

        registry->registerNativeFunction("__json_serialize", { nullptr, __json_serialize });
        registry->registerNativeFunction("__json_serializeWithOptions", { nullptr, __json_serializeWithOptions });
        registry->registerNativeFunction("__json_deserialize", { nullptr, __json_deserialize });
        registry->registerNativeFunction("__json_deserializeAs", { nullptr, __json_deserializeAs });
        registry->registerNativeFunction("__json_format", { nullptr, __json_format });
        registry->registerNativeFunction("__json_stringify", { nullptr, __json_stringify });
        registry->registerNativeFunction("__json_readFile", { nullptr, __json_readFile });
        registry->registerNativeFunction("__json_writeFile", { nullptr, __json_writeFile });
    }

    void JsonNatives::cleanup()
    {
    }

    // ========== Serialization ==========

    value::Value JsonNatives::__json_serialize(void* userData, NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__json_serialize");

        SerializationOptions opts;
        opts.prettyPrint = true;
        opts.includeStaticFields = false;

        JsonSerializer serializer(ctx.env, opts);
        return serializer.serialize(args[0]);
    }

    value::Value JsonNatives::__json_serializeWithOptions(void* userData, NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 3, "__json_serializeWithOptions");

        SerializationOptions opts;
        opts.includeStaticFields = extractBool(args[1], "__json_serializeWithOptions");
        opts.prettyPrint = extractBool(args[2], "__json_serializeWithOptions");

        JsonSerializer serializer(ctx.env, opts);
        return serializer.serialize(args[0]);
    }

    // ========== Deserialization ==========

    value::Value JsonNatives::__json_deserialize(void* userData, NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__json_deserialize");
        const std::string jsonStr = extractString(args[0], "__json_deserialize");

        JsonDeserializer deserializer(ctx.env);
        return deserializer.deserialize(jsonStr);
    }

    value::Value JsonNatives::__json_deserializeAs(void* userData, NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__json_deserializeAs");
        const std::string jsonStr = extractString(args[0], "__json_deserializeAs");
        const std::string className = extractString(args[1], "__json_deserializeAs");

        JsonDeserializer deserializer(ctx.env);
        return deserializer.deserializeAs(jsonStr, className);
    }

    // ========== JSON String Operations ==========

    value::Value JsonNatives::__json_format(void* userData, NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__json_format");
        const std::string jsonStr = extractString(args[0], "__json_format");

        auto parsed = JsonParser::parse(jsonStr);
        return parsed->toJsonString(true);
    }

    value::Value JsonNatives::__json_stringify(void* userData, NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__json_stringify");
        bool pretty = extractBool(args[1], "__json_stringify");

        SerializationOptions opts;
        opts.prettyPrint = pretty;
        opts.includeStaticFields = false;

        JsonSerializer serializer(ctx.env, opts);
        return serializer.serialize(args[0]);
    }

    // ========== File I/O ==========

    value::Value JsonNatives::__json_readFile(void* userData, NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__json_readFile");
        const std::string filePath = extractString(args[0], "__json_readFile");

        lexer::FileReader reader;
        return reader.readFile(filePath);
    }

    value::Value JsonNatives::__json_writeFile(void* userData, NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__json_writeFile");
        const std::string filePath = extractString(args[0], "__json_writeFile");
        const std::string content = extractString(args[1], "__json_writeFile");

        FileWriteNative::writeFile(filePath, content);
        return std::monostate{};
    }

    // ========== Helpers ==========

    void JsonNatives::validateArgCount(std::span<const value::Value> args,
                                       size_t expected, const std::string& funcName)
    {
        if (args.size() != expected)
        {
            throw errors::RuntimeException(
                funcName + " expects " + std::to_string(expected) +
                " argument(s), got " + std::to_string(args.size()));
        }
    }

    std::string JsonNatives::extractString(const value::Value& arg,
                                           const std::string& funcName)
    {
        if (value::isString(arg))
            return value::asString(arg);

        if (value::isInternedString(arg))
            return value::asInternedString(arg).getString();

        throw errors::RuntimeException(funcName + ": expected string argument");
    }

    bool JsonNatives::extractBool(const value::Value& arg,
                                   const std::string& funcName)
    {
        if (value::isBool(arg))
            return value::asBool(arg);

        throw errors::RuntimeException(funcName + ": expected bool argument");
    }
}


