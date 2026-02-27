#include "JsonSerializer.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../value/NativeArray.hpp"
#include "../value/ValueObject.hpp"
#include "../value/FlatMultiArray.hpp"
#include "../value/SparseMultiArray.hpp"
#include "../errors/RuntimeException.hpp"
#include <stdexcept>

namespace json
{
    JsonSerializer::JsonSerializer(std::shared_ptr<environment::Environment> env,
                                   const SerializationOptions& opts)
        : environment(std::move(env)), options(opts)
    {
    }

    std::string JsonSerializer::serialize(const value::Value& val)
    {
        visitedObjects.clear();
        currentDepth = 0;
        auto jsonVal = serializeValue(val);
        return jsonVal->toJsonString(options.prettyPrint);
    }

    std::shared_ptr<JsonValue> JsonSerializer::toJsonValue(const value::Value& val)
    {
        visitedObjects.clear();
        currentDepth = 0;
        return serializeValue(val);
    }

    std::shared_ptr<JsonValue> JsonSerializer::serializeValue(const value::Value& val)
    {
        return std::visit([this](const auto& v) -> std::shared_ptr<JsonValue>
        {
            using T = std::decay_t<decltype(v)>;

            if constexpr (std::is_same_v<T, int64_t>)
                return JsonValue::integer(v);
            else if constexpr (std::is_same_v<T, double>)
                return JsonValue::floating(v);
            else if constexpr (std::is_same_v<T, bool>)
                return JsonValue::boolean(v);
            else if constexpr (std::is_same_v<T, std::string>)
                return JsonValue::string(v);
            else if constexpr (std::is_same_v<T, value::InternedString>)
                return JsonValue::string(v.getString());
            else if constexpr (std::is_same_v<T, std::monostate>)
                return JsonValue::null();
            else if constexpr (std::is_same_v<T, nullptr_t>)
                return JsonValue::null();
            else if constexpr (std::is_same_v<T, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>)
                return v ? serializeObject(v) : JsonValue::null();
            else if constexpr (std::is_same_v<T, std::shared_ptr<value::ValueObject>>)
                return v ? serializeValueObject(v) : JsonValue::null();
            else if constexpr (std::is_same_v<T, std::shared_ptr<value::NativeArray>>)
                return v ? serializeArray(v) : JsonValue::null();
            else if constexpr (std::is_same_v<T, std::shared_ptr<value::FlatMultiArray>>)
                return JsonValue::null(); // Multi-dimensional array, not supported in JSON
            else if constexpr (std::is_same_v<T, std::shared_ptr<value::SparseMultiArray>>)
                return JsonValue::null(); // Sparse multi-array, not supported in JSON
            else if constexpr (std::is_same_v<T, std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>)
                return JsonValue::null(); // Object multi-array, not supported in JSON
            else
                return JsonValue::null(); // Lambda, Promise, etc.
        }, val);
    }

    std::shared_ptr<JsonValue> JsonSerializer::serializeObject(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj)
    {
        DepthGuard guard(*this);

        const void* ptr = obj.get();
        if (visitedObjects.count(ptr))
        {
            throw errors::RuntimeException("Circular reference detected while serializing object of type '"
                                            + obj->getTypeName() + "'");
        }
        visitedObjects.insert(ptr);

        auto jsonObj = JsonValue::object();
        jsonObj->setProperty("__type", JsonValue::string(obj->getTypeName()));

        // Generic type bindings
        const auto& bindings = obj->getGenericTypeBindings();
        if (!bindings.empty())
            addGenericBindings(bindings, jsonObj);

        // Instance fields
        const auto& fieldValues = obj->getAllFieldValues();
        for (const auto& [name, fieldVal] : fieldValues)
        {
            jsonObj->setProperty(name, serializeValue(fieldVal));
        }

        // Static fields
        if (options.includeStaticFields)
        {
            auto classDef = obj->getClassDefinition();
            if (classDef)
                addStaticFields(classDef, jsonObj);
        }

        visitedObjects.erase(ptr);
        return jsonObj;
    }

    std::shared_ptr<JsonValue> JsonSerializer::serializeValueObject(
        const std::shared_ptr<value::ValueObject>& obj)
    {
        DepthGuard guard(*this);

        auto jsonObj = JsonValue::object();
        auto classDef = obj->getClassDefinition();

        if (classDef)
            jsonObj->setProperty("__type", JsonValue::string(classDef->getClassName()));

        // Generic type bindings
        const auto& bindings = obj->getGenericTypeBindings();
        if (!bindings.empty())
            addGenericBindings(bindings, jsonObj);

        // Fields by name using ClassDefinition
        if (classDef)
        {
            const auto& fields = classDef->getInstanceFields();
            for (const auto& [name, fieldDef] : fields)
            {
                value::Value fieldVal = obj->getFieldValue(name);
                jsonObj->setProperty(name, serializeValue(fieldVal));
            }

            if (options.includeStaticFields)
                addStaticFields(classDef, jsonObj);
        }

        return jsonObj;
    }

    std::shared_ptr<JsonValue> JsonSerializer::serializeArray(
        const std::shared_ptr<value::NativeArray>& arr)
    {
        DepthGuard guard(*this);

        auto jsonArr = JsonValue::array();
        size_t len = arr->size();

        for (size_t i = 0; i < len; ++i)
        {
            value::Value elem = arr->get(i);
            jsonArr->addToArray(serializeValue(elem));
        }

        return jsonArr;
    }

    void JsonSerializer::addStaticFields(
        const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef,
        std::shared_ptr<JsonValue>& jsonObj)
    {
        const auto& staticFields = classDef->getStaticFields();
        if (staticFields.empty()) return;

        auto staticJson = JsonValue::object();
        for (const auto& [name, fieldDef] : staticFields)
        {
            const value::Value& val = fieldDef->getValue();
            staticJson->setProperty(name, serializeValue(val));
        }
        jsonObj->setProperty("__static", std::move(staticJson));
    }

    void JsonSerializer::addGenericBindings(
        const std::unordered_map<std::string, std::string>& bindings,
        std::shared_ptr<JsonValue>& jsonObj)
    {
        auto genJson = JsonValue::object();
        for (const auto& [param, type] : bindings)
        {
            genJson->setProperty(param, JsonValue::string(type));
        }
        jsonObj->setProperty("__generics", std::move(genJson));
    }

    // DepthGuard implementation
    JsonSerializer::DepthGuard::DepthGuard(JsonSerializer& s) : serializer(s)
    {
        serializer.currentDepth++;
        if (serializer.currentDepth > serializer.options.maxDepth)
        {
            throw errors::RuntimeException(
                "Maximum serialization depth (" + std::to_string(serializer.options.maxDepth)
                + ") exceeded. Possible circular reference or deeply nested structure.");
        }
    }

    JsonSerializer::DepthGuard::~DepthGuard()
    {
        serializer.currentDepth--;
    }
}
