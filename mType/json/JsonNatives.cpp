#include "JsonNatives.hpp"
#include "JsonSerializer.hpp"
#include "JsonDeserializer.hpp"
#include "JsonParser.hpp"
#include "FileWriteNative.hpp"
#include "../services/FileReader.hpp"
#include "../errors/RuntimeException.hpp"
#include <stdexcept>

namespace json
{
    std::shared_ptr<environment::Environment> JsonNatives::currentEnvironment = nullptr;

    void JsonNatives::registerAll(std::shared_ptr<environment::Environment> env)
    {
        currentEnvironment = env;
        auto registry = env->getNativeRegistry();

        registry->registerNativeFunction("__json_serialize", __json_serialize);
        registry->registerNativeFunction("__json_serializeWithOptions", __json_serializeWithOptions);
        registry->registerNativeFunction("__json_deserialize", __json_deserialize);
        registry->registerNativeFunction("__json_deserializeAs", __json_deserializeAs);
        registry->registerNativeFunction("__json_format", __json_format);
        registry->registerNativeFunction("__json_stringify", __json_stringify);
        registry->registerNativeFunction("__json_readFile", __json_readFile);
        registry->registerNativeFunction("__json_writeFile", __json_writeFile);
    }

    void JsonNatives::cleanup()
    {
        currentEnvironment = nullptr;
    }

    // ========== Serialization ==========

    value::Value JsonNatives::__json_serialize(const std::vector<value::Value>& args)
    {
        validateArgCount(args, 1, "__json_serialize");

        SerializationOptions opts;
        opts.prettyPrint = true;
        opts.includeStaticFields = false;

        JsonSerializer serializer(currentEnvironment, opts);
        return serializer.serialize(args[0]);
    }

    value::Value JsonNatives::__json_serializeWithOptions(const std::vector<value::Value>& args)
    {
        validateArgCount(args, 3, "__json_serializeWithOptions");

        SerializationOptions opts;
        opts.includeStaticFields = extractBool(args[1], "__json_serializeWithOptions");
        opts.prettyPrint = extractBool(args[2], "__json_serializeWithOptions");

        JsonSerializer serializer(currentEnvironment, opts);
        return serializer.serialize(args[0]);
    }

    // ========== Deserialization ==========

    value::Value JsonNatives::__json_deserialize(const std::vector<value::Value>& args)
    {
        validateArgCount(args, 1, "__json_deserialize");
        const std::string& jsonStr = extractString(args[0], "__json_deserialize");

        JsonDeserializer deserializer(currentEnvironment);
        return deserializer.deserialize(jsonStr);
    }

    value::Value JsonNatives::__json_deserializeAs(const std::vector<value::Value>& args)
    {
        validateArgCount(args, 2, "__json_deserializeAs");
        const std::string& jsonStr = extractString(args[0], "__json_deserializeAs");
        const std::string& className = extractString(args[1], "__json_deserializeAs");

        JsonDeserializer deserializer(currentEnvironment);
        return deserializer.deserializeAs(jsonStr, className);
    }

    // ========== JSON String Operations ==========

    value::Value JsonNatives::__json_format(const std::vector<value::Value>& args)
    {
        validateArgCount(args, 1, "__json_format");
        const std::string& jsonStr = extractString(args[0], "__json_format");

        auto parsed = JsonParser::parse(jsonStr);
        return parsed->toJsonString(true);
    }

    value::Value JsonNatives::__json_stringify(const std::vector<value::Value>& args)
    {
        validateArgCount(args, 2, "__json_stringify");
        bool pretty = extractBool(args[1], "__json_stringify");

        SerializationOptions opts;
        opts.prettyPrint = pretty;
        opts.includeStaticFields = false;

        JsonSerializer serializer(currentEnvironment, opts);
        return serializer.serialize(args[0]);
    }

    // ========== File I/O ==========

    value::Value JsonNatives::__json_readFile(const std::vector<value::Value>& args)
    {
        validateArgCount(args, 1, "__json_readFile");
        const std::string& filePath = extractString(args[0], "__json_readFile");

        lexer::FileReader reader;
        return reader.readFile(filePath);
    }

    value::Value JsonNatives::__json_writeFile(const std::vector<value::Value>& args)
    {
        validateArgCount(args, 2, "__json_writeFile");
        const std::string& filePath = extractString(args[0], "__json_writeFile");
        const std::string& content = extractString(args[1], "__json_writeFile");

        FileWriteNative::writeFile(filePath, content);
        return std::monostate{};
    }

    // ========== Helpers ==========

    void JsonNatives::validateArgCount(const std::vector<value::Value>& args,
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
        if (std::holds_alternative<std::string>(arg))
            return std::get<std::string>(arg);

        if (std::holds_alternative<value::InternedString>(arg))
            return std::get<value::InternedString>(arg).getString();

        throw errors::RuntimeException(funcName + ": expected string argument");
    }

    bool JsonNatives::extractBool(const value::Value& arg,
                                   const std::string& funcName)
    {
        if (std::holds_alternative<bool>(arg))
            return std::get<bool>(arg);

        throw errors::RuntimeException(funcName + ": expected bool argument");
    }
}
