// MYT-126: walled off under flag-on — variant accessors not migrated.
#ifndef MTYPE_TAGGED_VALUE
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

namespace
{
    template<typename T>
    T safeGet(const value::Value& val, const std::string& context)
    {
        if (!std::holds_alternative<T>(val))
            throw errors::RuntimeException("Unexpected value type while " + context);
        return std::get<T>(val);
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
        if (std::holds_alternative<nullptr_t>(headIt->second) ||
            std::holds_alternative<std::monostate>(headIt->second))
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

            if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(nextIt->second))
                node = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(nextIt->second);
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

#endif
