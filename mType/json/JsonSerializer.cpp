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
        const auto& fieldValues = obj->getAllFieldValues();
        for (const auto& [name, fieldVal] : fieldValues)
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

        const auto& fields = obj->getAllFieldValues();
        auto countIt = fields.find(countField);
        auto dataIt = fields.find(dataField);
        if (countIt == fields.end() || dataIt == fields.end())
            return jsonArr;

        int64_t count = safeGet<int64_t>(countIt->second,
            "serializing list collection: '" + countField + "' is not an int");
        auto arr = safeGet<std::shared_ptr<value::NativeArray>>(dataIt->second,
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

        const auto& fields = obj->getAllFieldValues();
        auto topIt = fields.find("top");
        auto dataIt = fields.find("data");
        if (topIt == fields.end() || dataIt == fields.end())
            return jsonArr;

        int64_t top = safeGet<int64_t>(topIt->second,
            "serializing Stack: 'top' is not an int");
        auto arr = safeGet<std::shared_ptr<value::NativeArray>>(dataIt->second,
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

        const auto& fields = obj->getAllFieldValues();
        auto frontIt = fields.find("front");
        auto countIt = fields.find("count");
        auto capIt = fields.find("capacity");
        auto dataIt = fields.find("data");
        if (frontIt == fields.end() || countIt == fields.end() ||
            capIt == fields.end() || dataIt == fields.end())
            return jsonArr;

        int64_t front = safeGet<int64_t>(frontIt->second,
            "serializing ArrayQueue: 'front' is not an int");
        int64_t count = safeGet<int64_t>(countIt->second,
            "serializing ArrayQueue: 'count' is not an int");
        int64_t capacity = safeGet<int64_t>(capIt->second,
            "serializing ArrayQueue: 'capacity' is not an int");
        auto arr = safeGet<std::shared_ptr<value::NativeArray>>(dataIt->second,
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

        const auto& fields = obj->getAllFieldValues();
        auto headIt = fields.find("head");
        if (headIt == fields.end()) return jsonArr;

        // head could be null
        if (value::isNullType(headIt->second) || value::isVoid(headIt->second))
            return jsonArr;

        auto node = safeGet<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(headIt->second,
            "serializing LinkedList: 'head' is not an ObjectInstance");
        while (node)
        {
            const auto& nodeFields = node->getAllFieldValues();
            auto dataIt = nodeFields.find("data");
            if (dataIt != nodeFields.end())
                jsonArr->addToArray(serializeValue(dataIt->second));

            auto nextIt = nodeFields.find("next");
            if (nextIt == nodeFields.end()) break;

            if (value::isObject(nextIt->second))
                node = value::asObject(nextIt->second);
            else
                break;
        }

        return jsonArr;
    }

    std::shared_ptr<JsonValue> JsonSerializer::serializeHashMap(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj)
    {
        auto jsonArr = JsonValue::array();

        const auto& fields = obj->getAllFieldValues();
        auto capIt = fields.find("capacity");
        auto sizesIt = fields.find("bucketSizes");
        auto keysIt = fields.find("keyBuckets");
        auto valsIt = fields.find("valueBuckets");
        if (capIt == fields.end() || sizesIt == fields.end() ||
            keysIt == fields.end() || valsIt == fields.end())
            return jsonArr;

        int64_t capacity = safeGet<int64_t>(capIt->second,
            "serializing HashMap: 'capacity' is not an int");
        auto bucketSizes = safeGet<std::shared_ptr<value::NativeArray>>(sizesIt->second,
            "serializing HashMap: 'bucketSizes' is not an array");
        // 2D bucket arrays are jagged NativeArrays (NativeArray of NativeArrays)
        auto keyBuckets = safeGet<std::shared_ptr<value::NativeArray>>(keysIt->second,
            "serializing HashMap: 'keyBuckets' is not an array");
        auto valBuckets = safeGet<std::shared_ptr<value::NativeArray>>(valsIt->second,
            "serializing HashMap: 'valueBuckets' is not an array");
        if (!bucketSizes || !keyBuckets || !valBuckets) return jsonArr;

        for (int64_t b = 0; b < capacity; ++b)
        {
            int64_t bSize = safeGet<int64_t>(bucketSizes->get(static_cast<size_t>(b)),
                "serializing HashMap: bucketSizes element is not an int");
            if (bSize <= 0) continue;

            auto keyRow = safeGet<std::shared_ptr<value::NativeArray>>(
                keyBuckets->get(static_cast<size_t>(b)),
                "serializing HashMap: keyBuckets row is not an array");
            auto valRow = safeGet<std::shared_ptr<value::NativeArray>>(
                valBuckets->get(static_cast<size_t>(b)),
                "serializing HashMap: valueBuckets row is not an array");
            if (!keyRow || !valRow) continue;

            for (int64_t j = 0; j < bSize; ++j)
            {
                auto entry = JsonValue::object();
                entry->setProperty("key", serializeValue(keyRow->get(static_cast<size_t>(j))));
                entry->setProperty("value", serializeValue(valRow->get(static_cast<size_t>(j))));
                jsonArr->addToArray(std::move(entry));
            }
        }

        return jsonArr;
    }

    std::shared_ptr<JsonValue> JsonSerializer::serializeHashSet(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj)
    {
        auto jsonArr = JsonValue::array();

        const auto& fields = obj->getAllFieldValues();
        auto capIt = fields.find("capacity");
        auto sizesIt = fields.find("bucketSizes");
        auto bucketsIt = fields.find("buckets");
        if (capIt == fields.end() || sizesIt == fields.end() || bucketsIt == fields.end())
            return jsonArr;

        int64_t capacity = safeGet<int64_t>(capIt->second,
            "serializing HashSet: 'capacity' is not an int");
        auto bucketSizes = safeGet<std::shared_ptr<value::NativeArray>>(sizesIt->second,
            "serializing HashSet: 'bucketSizes' is not an array");
        // 2D bucket array is a jagged NativeArray (NativeArray of NativeArrays)
        auto buckets = safeGet<std::shared_ptr<value::NativeArray>>(bucketsIt->second,
            "serializing HashSet: 'buckets' is not an array");
        if (!bucketSizes || !buckets) return jsonArr;

        for (int64_t b = 0; b < capacity; ++b)
        {
            int64_t bSize = safeGet<int64_t>(bucketSizes->get(static_cast<size_t>(b)),
                "serializing HashSet: bucketSizes element is not an int");
            if (bSize <= 0) continue;

            auto row = safeGet<std::shared_ptr<value::NativeArray>>(
                buckets->get(static_cast<size_t>(b)),
                "serializing HashSet: buckets row is not an array");
            if (!row) continue;

            for (int64_t j = 0; j < bSize; ++j)
                jsonArr->addToArray(serializeValue(row->get(static_cast<size_t>(j))));
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
