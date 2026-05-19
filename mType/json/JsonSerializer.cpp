#include "JsonSerializer.hpp"
#include <cstddef>
#include <cstdint>
#include "../value/ObjectInstance.hpp"
#include "../environment/registry/ClassDefinition.hpp"
#include "../environment/registry/FieldDefinition.hpp"
#include "../value/NativeArray.hpp"
#include "../value/ValueObject.hpp"
#include "../value/ValueShim.hpp"
#include "../errors/RuntimeException.hpp"
#include <stdexcept>

namespace
{
    void checkReservedFieldName(const std::string& fieldName, const std::string& className)
    {
        if (fieldName.size() >= 2 && fieldName[0] == '_' && fieldName[1] == '_')
        {
            throw errors::RuntimeException(
                "Cannot serialize class '" + className + "': field '" + fieldName
                + "' conflicts with reserved JSON metadata prefix '__'");
        }
    }
}

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
        if (value::isVoid(val) || value::isNullType(val))
            return JsonValue::null();
        if (value::isInt(val))
            return JsonValue::integer(value::asInt(val));
        if (value::isFloat(val))
            return JsonValue::floating(value::asFloat(val));
        if (value::isBool(val))
            return JsonValue::boolean(value::asBool(val));
        // MYT-317: SSO-aware. Folds the three string forms (STD_STRING heap,
        // INTERNED_STRING heap, STRING_INLINE) into a single branch.
        if (value::isAnyString(val))
            return JsonValue::string(std::string(value::asStringView(val)));
        if (value::isObject(val))
        {
            auto p = value::asObject(val);
            return p ? serializeObject(p) : JsonValue::null();
        }
        if (value::isValueObject(val))
        {
            auto p = value::asValueObject(val);
            return p ? serializeValueObject(p) : JsonValue::null();
        }
        if (value::isNativeArray(val))
        {
            auto p = value::asNativeArray(val);
            return p ? serializeArray(p) : JsonValue::null();
        }
        // FlatMultiArray / SparseMultiArray / FlatMultiObjectArray / Lambda /
        // Promise — not representable in JSON.
        return JsonValue::null();
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
        CycleGuard cycleGuard(visitedObjects, ptr);

        if (isCollectionType(obj->getTypeName()))
            return serializeCollection(obj);

        auto jsonObj = JsonValue::object();
        jsonObj->setProperty("__type", JsonValue::string(obj->getTypeName()));

        const auto& bindings = obj->getGenericTypeBindings();
        if (!bindings.empty())
            addGenericBindings(bindings, jsonObj);

        for (const auto& [name, fieldVal] : obj->getAllFields())
        {
            checkReservedFieldName(name, obj->getTypeName());
            jsonObj->setProperty(name, serializeValue(fieldVal));
        }

        if (options.includeStaticFields)
        {
            auto classDef = obj->getClassDefinition();
            if (classDef)
                addStaticFields(classDef, jsonObj);
        }

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

        const auto& bindings = obj->getGenericTypeBindings();
        if (!bindings.empty())
            addGenericBindings(bindings, jsonObj);

        if (classDef)
        {
            const auto& fields = classDef->getInstanceFields();
            for (const auto& [name, fieldDef] : fields)
            {
                checkReservedFieldName(name, classDef->getClassName());
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

    bool JsonSerializer::isCollectionType(const std::string& typeName)
    {
        return typeName == "ArrayList" || typeName == "HashMap" ||
               typeName == "HashSet" || typeName == "LinkedList" ||
               typeName == "Stack" || typeName == "ArrayQueue";
    }

    JsonSerializer::DepthGuard::DepthGuard(JsonSerializer& s) : serializer(s)
    {
        if (serializer.currentDepth >= serializer.options.maxDepth)
        {
            throw errors::RuntimeException(
                "Maximum serialization depth (" + std::to_string(serializer.options.maxDepth)
                + ") exceeded. Possible circular reference or deeply nested structure.");
        }
        serializer.currentDepth++;
    }

    JsonSerializer::DepthGuard::~DepthGuard()
    {
        serializer.currentDepth--;
    }

    JsonSerializer::CycleGuard::CycleGuard(std::unordered_set<const void*>& visited, const void* p)
        : visited(visited), ptr(p)
    {
        visited.insert(ptr);
    }

    JsonSerializer::CycleGuard::~CycleGuard()
    {
        visited.erase(ptr);
    }
}
