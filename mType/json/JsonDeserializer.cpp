#include "JsonDeserializer.hpp"
#include "JsonParser.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../value/NativeArray.hpp"
#include "../value/ValueObject.hpp"
#include "../errors/RuntimeException.hpp"
#include "../ast/GenericType.hpp"
#include <stdexcept>

namespace json
{
    JsonDeserializer::JsonDeserializer(std::shared_ptr<environment::Environment> env)
        : environment(std::move(env))
    {
    }

    value::Value JsonDeserializer::deserialize(const std::string& jsonString)
    {
        currentDepth = 0;
        auto jsonVal = JsonParser::parse(jsonString);
        return fromJsonValue(jsonVal);
    }

    value::Value JsonDeserializer::deserializeAs(const std::string& jsonString,
                                                  const std::string& className)
    {
        currentDepth = 0;
        auto jsonVal = JsonParser::parse(jsonString);

        if (jsonVal->isObject())
            return deserializeObjectAs(jsonVal, className);

        return fromJsonValue(jsonVal);
    }

    value::Value JsonDeserializer::fromJsonValue(const std::shared_ptr<JsonValue>& json)
    {
        if (!json || json->isNull())
            return nullptr;

        switch (json->getType())
        {
            case JsonType::BOOLEAN:
                return json->asBool();
            case JsonType::INTEGER:
                return json->asInt();
            case JsonType::FLOAT:
                return json->asFloat();
            case JsonType::STRING:
                return json->asString();
            case JsonType::ARRAY:
                return deserializeArray(json);
            case JsonType::OBJECT:
                return deserializeObject(json);
            default:
                return nullptr;
        }
    }

    value::Value JsonDeserializer::deserializeObject(const std::shared_ptr<JsonValue>& json)
    {
        if (!json->hasProperty("__type"))
        {
            throw errors::RuntimeException(
                "Cannot deserialize untyped JSON object. "
                "Use deserializeAs() to specify target class, "
                "or ensure JSON contains '__type' metadata.");
        }

        auto typeProp = json->getProperty("__type");
        std::string className = typeProp->asString();

        return deserializeObjectAs(json, className);
    }

    value::Value JsonDeserializer::deserializeObjectAs(
        const std::shared_ptr<JsonValue>& json,
        const std::string& className)
    {
        DepthGuard guard(*this);

        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::RuntimeException(
                "Unknown class '" + className + "' during JSON deserialization");
        }

        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);

        // Restore generic bindings first (needed for typed field resolution)
        if (json->hasProperty("__generics"))
            applyGenericBindings(instance, json->getProperty("__generics"));

        // Populate instance fields
        populateInstanceFields(instance, classDef, json);

        // Also populate inherited fields
        auto parentDef = classDef->getParentClass();
        while (parentDef)
        {
            populateInstanceFields(instance, parentDef, json);
            parentDef = parentDef->getParentClass();
        }

        // Restore static fields if present
        if (json->hasProperty("__static"))
            populateStaticFields(classDef, json->getProperty("__static"));

        return instance;
    }

    value::Value JsonDeserializer::deserializeArray(
        const std::shared_ptr<JsonValue>& json,
        const std::string& elementType)
    {
        DepthGuard guard(*this);

        const auto& arr = json->asArray();
        auto nativeArr = std::make_shared<value::NativeArray>(arr.size());

        for (size_t i = 0; i < arr.size(); ++i)
        {
            value::Value elem;
            if (!elementType.empty())
                elem = convertToFieldType(arr[i], elementType);
            else
                elem = fromJsonValue(arr[i]);
            nativeArr->set(i, elem);
        }

        return nativeArr;
    }

    void JsonDeserializer::populateInstanceFields(
        std::shared_ptr<runtimeTypes::klass::ObjectInstance>& instance,
        const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef,
        const std::shared_ptr<JsonValue>& json)
    {
        const auto& fields = classDef->getInstanceFields();

        for (const auto& [name, fieldDef] : fields)
        {
            if (!json->hasProperty(name)) continue;

            auto fieldJson = json->getProperty(name);
            std::string fieldType = resolveFieldType(fieldDef, instance);
            value::Value val = convertToFieldType(fieldJson, fieldType);
            instance->setField(name, val);
        }
    }

    void JsonDeserializer::populateStaticFields(
        const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef,
        const std::shared_ptr<JsonValue>& staticJson)
    {
        if (!staticJson || !staticJson->isObject()) return;

        const auto& staticFields = classDef->getStaticFields();

        for (const auto& [name, fieldDef] : staticFields)
        {
            if (!staticJson->hasProperty(name)) continue;

            auto fieldJson = staticJson->getProperty(name);
            std::string fieldType = resolveFieldTypeFromDef(fieldDef);
            value::Value val = convertToFieldType(fieldJson, fieldType);
            fieldDef->setValue(val);
        }
    }

    void JsonDeserializer::applyGenericBindings(
        std::shared_ptr<runtimeTypes::klass::ObjectInstance>& instance,
        const std::shared_ptr<JsonValue>& genJson)
    {
        if (!genJson || !genJson->isObject()) return;

        for (const auto& [param, typeVal] : genJson->asObject())
        {
            if (typeVal && typeVal->isString())
                instance->setGenericTypeBinding(param, typeVal->asString());
        }
    }

    std::string JsonDeserializer::resolveFieldType(
        const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field,
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& instance)
    {
        auto genType = field->getGenericType();
        if (genType)
        {
            // If it's a generic parameter (T, E, etc.), resolve via instance bindings
            if (genType->isGenericParameter())
            {
                std::string paramName = genType->getGenericName();
                std::string resolved = instance->resolveGenericType(paramName);
                if (!resolved.empty() && resolved != paramName)
                    return resolved;
            }
            return genType->getBaseTypeName();
        }

        // Fallback: use ValueType
        return valueTypeToString(field->getType());
    }

    value::Value JsonDeserializer::convertToFieldType(
        const std::shared_ptr<JsonValue>& json,
        const std::string& targetType)
    {
        if (!json || json->isNull())
            return nullptr;

        // Primitive types
        if (targetType == "int" || targetType == "Int")
            return json->isNumber() ? json->asInt() : fromJsonValue(json);
        if (targetType == "float" || targetType == "Float")
            return json->isNumber() ? json->asFloat() : fromJsonValue(json);
        if (targetType == "bool" || targetType == "Bool")
            return json->isBool() ? json->asBool() : fromJsonValue(json);
        if (targetType == "string" || targetType == "String")
            return json->isString() ? value::Value(json->asString()) : fromJsonValue(json);

        // Array types (e.g., "int[]", "Person[]")
        if (targetType.size() > 2 && targetType.substr(targetType.size() - 2) == "[]")
        {
            std::string elemType = targetType.substr(0, targetType.size() - 2);
            if (json->isArray())
                return deserializeArray(json, elemType);
            return fromJsonValue(json);
        }

        // Object type: try to deserialize as class
        if (json->isObject())
        {
            if (json->hasProperty("__type"))
                return deserializeObject(json);
            return deserializeObjectAs(json, targetType);
        }

        // Fallback
        return fromJsonValue(json);
    }

    std::string JsonDeserializer::resolveFieldTypeFromDef(
        const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field)
    {
        auto genType = field->getGenericType();
        if (genType)
            return genType->getBaseTypeName();
        return valueTypeToString(field->getType());
    }

    std::string JsonDeserializer::valueTypeToString(value::ValueType type)
    {
        switch (type)
        {
            case value::ValueType::INT:    return "int";
            case value::ValueType::FLOAT:  return "float";
            case value::ValueType::BOOL:   return "bool";
            case value::ValueType::STRING: return "string";
            case value::ValueType::OBJECT: return "Object";
            case value::ValueType::ARRAY:  return "Array";
            default: return "Object";
        }
    }

    // DepthGuard
    JsonDeserializer::DepthGuard::DepthGuard(JsonDeserializer& d) : deserializer(d)
    {
        if (deserializer.currentDepth >= MAX_DEPTH)
        {
            throw errors::RuntimeException(
                "Maximum deserialization depth (" + std::to_string(MAX_DEPTH)
                + ") exceeded.");
        }
        deserializer.currentDepth++;
    }

    JsonDeserializer::DepthGuard::~DepthGuard()
    {
        deserializer.currentDepth--;
    }
}
