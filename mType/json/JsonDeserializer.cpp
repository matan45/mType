#include "JsonDeserializer.hpp"
#include <cstddef>
#include <cstdint>
#include "JsonParser.hpp"
#include "../value/ObjectInstance.hpp"
#include "../environment/registry/ClassDefinition.hpp"
#include "../environment/registry/FieldDefinition.hpp"
#include "../value/NativeArray.hpp"
#include "../value/ObjectInstancePool.hpp"
#include "../value/ValueObject.hpp"
#include "../value/StringPool.hpp"
#include "../value/ValueShim.hpp"
#include "../errors/RuntimeException.hpp"
#include "../types/UnifiedType.hpp"

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
                return value::StringPool::getInstance().intern(json->asString());
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

        if (json->hasProperty("__collection"))
            return deserializeCollection(json, className);

        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::RuntimeException(
                "Unknown class '" + className + "' during JSON deserialization");
        }

        auto instance = value::ObjectInstancePool::getInstance().acquire(classDef);

        // Restore generic bindings first — needed for typed field resolution.
        if (json->hasProperty("__generics"))
            applyGenericBindings(instance, json->getProperty("__generics"));

        populateInstanceFields(instance, classDef, json);

        // Walk parents so inherited fields are populated too.
        auto parentDef = classDef->getParentClass();
        while (parentDef)
        {
            populateInstanceFields(instance, parentDef, json);
            parentDef = parentDef->getParentClass();
        }

        if (json->hasProperty("__static"))
            populateStaticFields(classDef, json->getProperty("__static"));

        // Value classes need ValueObject, not ObjectInstance.
        if (classDef->isValueClass())
        {
            auto valueObj = std::make_shared<value::ValueObject>(classDef);
            const auto& fieldIndexMap = classDef->getFieldIndexMap();
            for (const auto& [name, index] : fieldIndexMap)
                valueObj->setFieldByIndex(index, instance->getFieldValue(name));
            for (const auto& [param, type] : instance->getGenericTypeBindings())
                valueObj->setGenericTypeBinding(param, type);
            return valueObj;
        }

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
            value::Value val = convertToFieldType(fieldJson, fieldType, "field '" + name + "'");
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
            value::Value val = convertToFieldType(fieldJson, fieldType, "static field '" + name + "'");
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
        auto uType = field->getUnifiedType();
        if (uType)
        {
            // If it's a generic parameter (T, E, etc.), resolve via instance bindings.
            if (uType->isGenericParameter())
            {
                std::string paramName = uType->getName();
                std::string resolved = instance->resolveGenericType(paramName);
                if (!resolved.empty() && resolved != paramName)
                    return resolved;
            }
            return uType->getName();
        }

        // Fallback: use ValueType.
        return valueTypeToString(field->getType());
    }

    value::Value JsonDeserializer::convertToFieldType(
        const std::shared_ptr<JsonValue>& json,
        const std::string& targetType,
        const std::string& context)
    {
        if (!json || json->isNull())
            return nullptr;

        if (targetType == "int")
        {
            if (json->isNumber()) return json->asInt();
            throwTypeMismatch(json, targetType, context);
        }
        if (targetType == "float")
        {
            if (json->isNumber()) return json->asFloat();
            throwTypeMismatch(json, targetType, context);
        }
        if (targetType == "bool")
        {
            if (json->isBool()) return json->asBool();
            throwTypeMismatch(json, targetType, context);
        }
        if (targetType == "string")
        {
            if (json->isString())
                return value::Value(value::StringPool::getInstance().intern(json->asString()));
            throwTypeMismatch(json, targetType, context);
        }

        // Array types (e.g., "int[]", "Person[]").
        if (targetType.size() > 2 && targetType.substr(targetType.size() - 2) == "[]")
        {
            std::string elemType = targetType.substr(0, targetType.size() - 2);
            if (json->isArray())
                return deserializeArray(json, elemType);
            throwTypeMismatch(json, targetType, context);
        }
        if (targetType == "Array" || targetType == "array")
        {
            if (json->isArray())
                return deserializeArray(json);
            throwTypeMismatch(json, targetType, context);
        }

        // Object type: try to deserialize as class.
        if (json->isObject())
        {
            if (json->hasProperty("__type"))
                return deserializeObject(json);
            return deserializeObjectAs(json, targetType);
        }

        throwTypeMismatch(json, targetType, context);
    }

    std::string JsonDeserializer::resolveFieldTypeFromDef(
        const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field)
    {
        auto uType = field->getUnifiedType();
        if (uType)
            return uType->getName();
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

    std::string JsonDeserializer::jsonTypeToString(JsonType type)
    {
        switch (type)
        {
            case JsonType::NULL_VALUE: return "null";
            case JsonType::BOOLEAN:    return "bool";
            case JsonType::INTEGER:    return "int";
            case JsonType::FLOAT:      return "float";
            case JsonType::STRING:     return "string";
            case JsonType::ARRAY:      return "array";
            case JsonType::OBJECT:     return "object";
            default: return "unknown";
        }
    }

    void JsonDeserializer::throwTypeMismatch(
        const std::shared_ptr<JsonValue>& json,
        const std::string& targetType,
        const std::string& context)
    {
        std::string message = "JSON deserialization type mismatch";
        if (!context.empty())
            message += " for " + context;
        message += ": expected " + targetType + ", got ";
        message += json ? jsonTypeToString(json->getType()) : "null";
        throw errors::RuntimeException(message);
    }

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
