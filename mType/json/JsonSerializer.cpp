#include "JsonSerializer.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../value/NativeArray.hpp"
#include "../value/ValueObject.hpp"
#include "../value/FlatMultiArray.hpp"
#include "../value/SparseMultiArray.hpp"
#include "../value/ValueShim.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../errors/RuntimeException.hpp"
#include <stdexcept>

namespace
{
    // MYT-189: ported from std::variant accessors to the ValueShim.
    // Only the two types used internally are supported.
    template<typename T>
    T safeGet(const value::Value& val, const std::string& context)
    {
        if constexpr (std::is_same_v<T, int64_t>)
        {
            if (!value::isInt(val))
                throw errors::RuntimeException("Unexpected value type while " + context);
            return value::asInt(val);
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<value::NativeArray>>)
        {
            if (!value::isNativeArray(val))
                throw errors::RuntimeException("Unexpected value type while " + context);
            return value::asNativeArray(val);
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>)
        {
            if (!value::isObject(val))
                throw errors::RuntimeException("Unexpected value type while " + context);
            return value::asObject(val);
        }
        else
        {
            static_assert(sizeof(T*) == 0,
                "safeGet only supports int64_t, shared_ptr<NativeArray>, shared_ptr<ObjectInstance>");
        }
    }

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
        if (value::isString(val))
            return JsonValue::string(value::asString(val));
        if (value::isInternedString(val))
            return JsonValue::string(value::asInternedString(val).getString());
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

        // Collection types get special handling
        if (isCollectionType(obj->getTypeName()))
            return serializeCollection(obj);

        auto jsonObj = JsonValue::object();
        jsonObj->setProperty("__type", JsonValue::string(obj->getTypeName()));

        // Generic type bindings
        const auto& bindings = obj->getGenericTypeBindings();
        if (!bindings.empty())
            addGenericBindings(bindings, jsonObj);

        // Instance fields
        for (const auto& [name, fieldVal] : obj->getAllFields())
        {
            checkReservedFieldName(name, obj->getTypeName());
            jsonObj->setProperty(name, serializeValue(fieldVal));
        }

        // Static fields
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

    // === Collection Serialization ===

    bool JsonSerializer::isCollectionType(const std::string& typeName)
    {
        return typeName == "ArrayList" || typeName == "HashMap" ||
               typeName == "HashSet" || typeName == "LinkedList" ||
               typeName == "Stack" || typeName == "ArrayQueue";
    }

    std::shared_ptr<JsonValue> JsonSerializer::serializeCollection(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj)
    {
        const std::string& typeName = obj->getTypeName();

        auto jsonObj = JsonValue::object();
        jsonObj->setProperty("__type", JsonValue::string(typeName));

        const auto& bindings = obj->getGenericTypeBindings();
        if (!bindings.empty())
            addGenericBindings(bindings, jsonObj);

        if (typeName == "ArrayList")
        {
            jsonObj->setProperty("__collection", JsonValue::string("list"));
            jsonObj->setProperty("elements", serializeListCollection(obj, "count", "data"));
        }
        else if (typeName == "Stack")
        {
            jsonObj->setProperty("__collection", JsonValue::string("list"));
            jsonObj->setProperty("elements", serializeStackCollection(obj));
        }
        else if (typeName == "ArrayQueue")
        {
            jsonObj->setProperty("__collection", JsonValue::string("list"));
            jsonObj->setProperty("elements", serializeQueueCollection(obj));
        }
        else if (typeName == "LinkedList")
        {
            jsonObj->setProperty("__collection", JsonValue::string("list"));
            jsonObj->setProperty("elements", serializeLinkedList(obj));
        }
        else if (typeName == "HashMap")
        {
            jsonObj->setProperty("__collection", JsonValue::string("map"));
            jsonObj->setProperty("entries", serializeHashMap(obj));
        }
        else if (typeName == "HashSet")
        {
            jsonObj->setProperty("__collection", JsonValue::string("set"));
            jsonObj->setProperty("elements", serializeHashSet(obj));
        }

        return jsonObj;
    }

    std::shared_ptr<JsonValue> JsonSerializer::serializeListCollection(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj,
        const std::string& countField, const std::string& dataField)
    {
        auto jsonArr = JsonValue::array();

        value::Value countVal = obj->getFieldValue(countField);
        value::Value dataVal = obj->getFieldValue(dataField);
        
        if (value::isVoid(countVal) || value::isVoid(dataVal))
            return jsonArr;

        int64_t count = safeGet<int64_t>(countVal,
            "serializing list collection: '" + countField + "' is not an int");
        auto arr = safeGet<std::shared_ptr<value::NativeArray>>(dataVal,
            "serializing list collection: '" + dataField + "' is not an array");
        if (!arr) return jsonArr;

        for (int64_t i = 0; i < count; ++i)
            jsonArr->addToArray(serializeValue(arr->get(static_cast<size_t>(i))));

        return jsonArr;
    }

    std::shared_ptr<JsonValue> JsonSerializer::serializeStackCollection(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj)
    {
        auto jsonArr = JsonValue::array();

        value::Value topVal = obj->getFieldValue("top");
        value::Value dataVal = obj->getFieldValue("data");
        
        if (value::isVoid(topVal) || value::isVoid(dataVal))
            return jsonArr;

        int64_t top = safeGet<int64_t>(topVal,
            "serializing Stack: 'top' is not an int");
        auto arr = safeGet<std::shared_ptr<value::NativeArray>>(dataVal,
            "serializing Stack: 'data' is not an array");
        if (!arr || top < 0) return jsonArr;

        for (int64_t i = 0; i <= top; ++i)
            jsonArr->addToArray(serializeValue(arr->get(static_cast<size_t>(i))));

        return jsonArr;
    }

    std::shared_ptr<JsonValue> JsonSerializer::serializeQueueCollection(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj)
    {
        auto jsonArr = JsonValue::array();

        value::Value frontVal = obj->getFieldValue("front");
        value::Value countVal = obj->getFieldValue("count");
        value::Value capVal = obj->getFieldValue("capacity");
        value::Value dataVal = obj->getFieldValue("data");
        
        if (value::isVoid(frontVal) || value::isVoid(countVal) ||
            value::isVoid(capVal) || value::isVoid(dataVal))
            return jsonArr;

        int64_t front = safeGet<int64_t>(frontVal,
            "serializing ArrayQueue: 'front' is not an int");
        int64_t count = safeGet<int64_t>(countVal,
            "serializing ArrayQueue: 'count' is not an int");
        int64_t capacity = safeGet<int64_t>(capVal,
            "serializing ArrayQueue: 'capacity' is not an int");
        auto arr = safeGet<std::shared_ptr<value::NativeArray>>(dataVal,
            "serializing ArrayQueue: 'data' is not an array");
        if (!arr) return jsonArr;

        for (int64_t i = 0; i < count; ++i)
        {
            int64_t index = (front + i) % capacity;
            jsonArr->addToArray(serializeValue(arr->get(static_cast<size_t>(index))));
        }

        return jsonArr;
    }

    std::shared_ptr<JsonValue> JsonSerializer::serializeLinkedList(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj)
    {
        auto jsonArr = JsonValue::array();

        value::Value headVal = obj->getFieldValue("head");
        if (value::isVoid(headVal)) return jsonArr;

        // head could be null
        if (value::isNullType(headVal))
            return jsonArr;

        auto node = safeGet<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(headVal,
            "serializing LinkedList: 'head' is not an ObjectInstance");
        while (node)
        {
            value::Value dataVal = node->getFieldValue("data");
            if (!value::isVoid(dataVal))
                jsonArr->addToArray(serializeValue(dataVal));

            value::Value nextVal = node->getFieldValue("next");
            if (value::isVoid(nextVal)) break;

            if (value::isObject(nextVal))
                node = value::asObject(nextVal);
            else
                break;
        }

        return jsonArr;
    }

    std::shared_ptr<JsonValue> JsonSerializer::serializeHashMap(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj)
    {
        auto jsonArr = JsonValue::array();

        if (auto* storage = obj->getSpecializedCollection())
        {
            auto keys = storage->materializeKeys(environment.get());
            auto values = storage->materializeValues();
            for (size_t i = 0; i < storage->size(); ++i)
            {
                auto entry = JsonValue::object();
                entry->setProperty("key", serializeValue(keys->get(i)));
                entry->setProperty("value", serializeValue(values->get(i)));
                jsonArr->addToArray(std::move(entry));
            }
            return jsonArr;
        }

        value::Value capVal = obj->getFieldValue("capacity");
        value::Value keysVal = obj->getFieldValue("keys");
        value::Value valsVal = obj->getFieldValue("values");

        if (value::isVoid(capVal) || value::isVoid(keysVal) || value::isVoid(valsVal))
            return jsonArr;

        int64_t capacity = safeGet<int64_t>(capVal,
            "serializing HashMap: 'capacity' is not an int");
        auto keys = safeGet<std::shared_ptr<value::NativeArray>>(keysVal,
            "serializing HashMap: 'keys' is not an array");
        auto values = safeGet<std::shared_ptr<value::NativeArray>>(valsVal,
            "serializing HashMap: 'values' is not an array");
        if (!keys || !values) return jsonArr;

        for (int64_t i = 0; i < capacity; ++i)
        {
            value::Value keyVal = keys->get(static_cast<size_t>(i));
            if (value::isNullType(keyVal)) continue;

            auto entry = JsonValue::object();
            entry->setProperty("key", serializeValue(keyVal));
            entry->setProperty("value", serializeValue(values->get(static_cast<size_t>(i))));
            jsonArr->addToArray(std::move(entry));
        }

        return jsonArr;
    }

    std::shared_ptr<JsonValue> JsonSerializer::serializeHashSet(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj)
    {
        auto jsonArr = JsonValue::array();

        if (auto* storage = obj->getSpecializedCollection())
        {
            auto elements = storage->materializeKeys(environment.get());
            for (size_t i = 0; i < storage->size(); ++i)
            {
                jsonArr->addToArray(serializeValue(elements->get(i)));
            }
            return jsonArr;
        }

        value::Value capVal = obj->getFieldValue("capacity");
        value::Value elemsVal = obj->getFieldValue("elements");

        if (value::isVoid(capVal) || value::isVoid(elemsVal))
            return jsonArr;

        int64_t capacity = safeGet<int64_t>(capVal,
            "serializing HashSet: 'capacity' is not an int");
        auto elements = safeGet<std::shared_ptr<value::NativeArray>>(elemsVal,
            "serializing HashSet: 'elements' is not an array");
        if (!elements) return jsonArr;

        for (int64_t i = 0; i < capacity; ++i)
        {
            value::Value elem = elements->get(static_cast<size_t>(i));
            if (value::isNullType(elem)) continue;
            jsonArr->addToArray(serializeValue(elem));
        }

        return jsonArr;
    }

    // DepthGuard implementation
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
