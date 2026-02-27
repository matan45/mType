#include "JsonDeserializer.hpp"
#include "JsonParser.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../value/NativeArray.hpp"
#include "../value/ValueObject.hpp"
#include "../value/StringPool.hpp"
#include "../errors/RuntimeException.hpp"
#include "../ast/GenericType.hpp"
#include <stdexcept>
#include <functional>

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

        // Collection types get special handling
        if (json->hasProperty("__collection"))
            return deserializeCollection(json, className);

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

        // Value classes need ValueObject, not ObjectInstance
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
            return json->isString() ? value::Value(value::StringPool::getInstance().intern(json->asString())) : fromJsonValue(json);

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

    // === Collection Deserialization ===

    value::Value JsonDeserializer::deserializeCollection(
        const std::shared_ptr<JsonValue>& json, const std::string& className)
    {
        std::string collType = json->getProperty("__collection")->asString();

        if (collType == "list")
            return deserializeListCollection(json, className);
        if (collType == "map")
            return deserializeHashMapCollection(json);
        if (collType == "set")
            return deserializeHashSetCollection(json);

        throw errors::RuntimeException(
            "Unknown collection type '" + collType + "' during JSON deserialization");
    }

    value::Value JsonDeserializer::deserializeListCollection(
        const std::shared_ptr<JsonValue>& json, const std::string& className)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
            throw errors::RuntimeException("Unknown class '" + className + "' during JSON deserialization");

        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);

        if (json->hasProperty("__generics"))
            applyGenericBindings(instance, json->getProperty("__generics"));

        if (!json->hasProperty("elements"))
            return instance;

        const auto& elements = json->getProperty("elements")->asArray();
        int64_t count = static_cast<int64_t>(elements.size());

        // Resolve element type from generic binding
        std::string elemType;
        const auto& bindings = instance->getGenericTypeBindings();
        auto tIt = bindings.find("T");
        if (tIt != bindings.end())
            elemType = tIt->second;

        // Build data array
        auto dataArr = std::make_shared<value::NativeArray>(elements.size());
        for (size_t i = 0; i < elements.size(); ++i)
        {
            value::Value elem = elemType.empty()
                ? fromJsonValue(elements[i])
                : convertToFieldType(elements[i], elemType);
            dataArr->set(i, elem);
        }

        if (className == "ArrayList")
        {
            instance->setField("data", dataArr);
            instance->setField("count", count);
            instance->setField("capacity", count);
        }
        else if (className == "Stack")
        {
            instance->setField("data", dataArr);
            instance->setField("top", count - 1);
            instance->setField("capacity", count);
        }
        else if (className == "ArrayQueue")
        {
            instance->setField("data", dataArr);
            instance->setField("front", int64_t(0));
            instance->setField("rear", count);
            instance->setField("count", count);
            instance->setField("capacity", count);
        }
        else if (className == "LinkedList")
        {
            return deserializeLinkedListCollection(json);
        }

        return instance;
    }

    value::Value JsonDeserializer::deserializeLinkedListCollection(
        const std::shared_ptr<JsonValue>& json)
    {
        auto classDef = environment->findClass("LinkedList");
        if (!classDef)
            throw errors::RuntimeException("Unknown class 'LinkedList' during JSON deserialization");

        auto nodeDef = environment->findClass("Node");
        if (!nodeDef)
            throw errors::RuntimeException("Unknown class 'Node' during JSON deserialization");

        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);

        if (json->hasProperty("__generics"))
            applyGenericBindings(instance, json->getProperty("__generics"));

        if (!json->hasProperty("elements"))
        {
            instance->setField("head", nullptr);
            instance->setField("tail", nullptr);
            instance->setField("count", int64_t(0));
            return instance;
        }

        const auto& elements = json->getProperty("elements")->asArray();
        if (elements.empty())
        {
            instance->setField("head", nullptr);
            instance->setField("tail", nullptr);
            instance->setField("count", int64_t(0));
            return instance;
        }

        std::string elemType;
        const auto& bindings = instance->getGenericTypeBindings();
        auto tIt = bindings.find("T");
        if (tIt != bindings.end())
            elemType = tIt->second;

        // Build node chain
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> headNode;
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> prevNode;

        for (size_t i = 0; i < elements.size(); ++i)
        {
            auto node = std::make_shared<runtimeTypes::klass::ObjectInstance>(nodeDef);
            if (!elemType.empty())
                node->setGenericTypeBinding("T", elemType);

            value::Value elemVal = elemType.empty()
                ? fromJsonValue(elements[i])
                : convertToFieldType(elements[i], elemType);

            node->setField("data", elemVal);
            node->setField("next", nullptr);
            node->setField("prev", prevNode ? value::Value(prevNode) : value::Value(nullptr));

            if (prevNode)
                prevNode->setField("next", value::Value(node));

            if (i == 0)
                headNode = node;

            prevNode = node;
        }

        instance->setField("head", value::Value(headNode));
        instance->setField("tail", value::Value(prevNode));
        instance->setField("count", static_cast<int64_t>(elements.size()));

        return instance;
    }

    int64_t JsonDeserializer::computeHashCode(const value::Value& val)
    {
        return std::visit([](const auto& v) -> int64_t
        {
            using T = std::decay_t<decltype(v)>;

            if constexpr (std::is_same_v<T, int64_t>)
                return static_cast<int64_t>(std::hash<int64_t>{}(v) & 0x7FFFFFFF);
            else if constexpr (std::is_same_v<T, double>)
                return static_cast<int64_t>(std::hash<double>{}(v) & 0x7FFFFFFF);
            else if constexpr (std::is_same_v<T, bool>)
                return v ? 1231 : 1237;
            else if constexpr (std::is_same_v<T, std::string>)
                return static_cast<int64_t>(std::hash<std::string>{}(v) & 0x7FFFFFFF);
            else if constexpr (std::is_same_v<T, value::InternedString>)
                return static_cast<int64_t>(std::hash<std::string>{}(v.getString()) & 0x7FFFFFFF);
            else if constexpr (std::is_same_v<T, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>)
            {
                // Wrapper types: extract primitive value field
                if (!v) return 0;
                const auto& fields = v->getAllFieldValues();
                auto valIt = fields.find("value");
                if (valIt == fields.end()) return 0;

                // Recurse on the inner value
                return std::visit([](const auto& inner) -> int64_t
                {
                    using U = std::decay_t<decltype(inner)>;
                    if constexpr (std::is_same_v<U, int64_t>)
                        return static_cast<int64_t>(std::hash<int64_t>{}(inner) & 0x7FFFFFFF);
                    else if constexpr (std::is_same_v<U, double>)
                        return static_cast<int64_t>(std::hash<double>{}(inner) & 0x7FFFFFFF);
                    else if constexpr (std::is_same_v<U, bool>)
                        return inner ? 1231 : 1237;
                    else if constexpr (std::is_same_v<U, std::string>)
                        return static_cast<int64_t>(std::hash<std::string>{}(inner) & 0x7FFFFFFF);
                    else if constexpr (std::is_same_v<U, value::InternedString>)
                        return static_cast<int64_t>(std::hash<std::string>{}(inner.getString()) & 0x7FFFFFFF);
                    else
                        return 0;
                }, valIt->second);
            }
            else if constexpr (std::is_same_v<T, std::shared_ptr<value::ValueObject>>)
            {
                // Value types: extract primitive value field
                if (!v) return 0;
                value::Value inner = v->getFieldValue("value");
                return std::visit([](const auto& iv) -> int64_t
                {
                    using U = std::decay_t<decltype(iv)>;
                    if constexpr (std::is_same_v<U, int64_t>)
                        return static_cast<int64_t>(std::hash<int64_t>{}(iv) & 0x7FFFFFFF);
                    else if constexpr (std::is_same_v<U, double>)
                        return static_cast<int64_t>(std::hash<double>{}(iv) & 0x7FFFFFFF);
                    else if constexpr (std::is_same_v<U, bool>)
                        return iv ? 1231 : 1237;
                    else if constexpr (std::is_same_v<U, std::string>)
                        return static_cast<int64_t>(std::hash<std::string>{}(iv) & 0x7FFFFFFF);
                    else if constexpr (std::is_same_v<U, value::InternedString>)
                        return static_cast<int64_t>(std::hash<std::string>{}(iv.getString()) & 0x7FFFFFFF);
                    else
                        return 0;
                }, inner);
            }
            else
                return 0;
        }, val);
    }

    int64_t JsonDeserializer::computeBucketIndex(int64_t hash, int64_t capacity)
    {
        // Match mType HashMap.getBucketIndex() exactly
        if (hash < 0) { hash = -hash; if (hash < 0) hash += 1; }
        hash = hash * 1610612741;
        hash = hash + (hash / capacity);
        if (hash < 0) hash = -hash;
        return hash % capacity;
    }

    value::Value JsonDeserializer::deserializeHashMapCollection(
        const std::shared_ptr<JsonValue>& json)
    {
        auto classDef = environment->findClass("HashMap");
        if (!classDef)
            throw errors::RuntimeException("Unknown class 'HashMap' during JSON deserialization");

        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);

        if (json->hasProperty("__generics"))
            applyGenericBindings(instance, json->getProperty("__generics"));

        std::string keyType, valType;
        const auto& bindings = instance->getGenericTypeBindings();
        auto kIt = bindings.find("K");
        auto vIt = bindings.find("V");
        if (kIt != bindings.end()) keyType = kIt->second;
        if (vIt != bindings.end()) valType = vIt->second;

        if (!json->hasProperty("entries"))
        {
            // Empty map with default jagged NativeArray structure
            int64_t capacity = 16;
            int64_t bucketWidth = 4;
            auto keyBuckets = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
            auto valBuckets = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
            auto bucketSizes = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
            for (size_t i = 0; i < static_cast<size_t>(capacity); ++i)
            {
                keyBuckets->set(i, std::make_shared<value::NativeArray>(static_cast<size_t>(bucketWidth)));
                valBuckets->set(i, std::make_shared<value::NativeArray>(static_cast<size_t>(bucketWidth)));
                bucketSizes->set(i, int64_t(0));
            }

            instance->setField("keyBuckets", keyBuckets);
            instance->setField("valueBuckets", valBuckets);
            instance->setField("bucketSizes", bucketSizes);
            instance->setField("capacity", capacity);
            instance->setField("count", int64_t(0));
            return instance;
        }

        const auto& entries = json->getProperty("entries")->asArray();
        int64_t count = static_cast<int64_t>(entries.size());

        // Choose capacity to avoid triggering resize (count > capacity * 3/4)
        int64_t capacity = 16;
        while (count > capacity * 3 / 4) capacity *= 2;

        int64_t bucketWidth = 4;
        // Create jagged NativeArray structure (NativeArray of NativeArrays) to match VM runtime
        auto keyBuckets = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
        auto valBuckets = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
        auto bucketSizes = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
        for (size_t i = 0; i < static_cast<size_t>(capacity); ++i)
        {
            keyBuckets->set(i, std::make_shared<value::NativeArray>(static_cast<size_t>(bucketWidth)));
            valBuckets->set(i, std::make_shared<value::NativeArray>(static_cast<size_t>(bucketWidth)));
            bucketSizes->set(i, int64_t(0));
        }

        for (size_t i = 0; i < entries.size(); ++i)
        {
            auto entry = entries[i];
            value::Value key = keyType.empty()
                ? fromJsonValue(entry->getProperty("key"))
                : convertToFieldType(entry->getProperty("key"), keyType);
            value::Value val = valType.empty()
                ? fromJsonValue(entry->getProperty("value"))
                : convertToFieldType(entry->getProperty("value"), valType);

            int64_t hash = computeHashCode(key);
            int64_t bucket = computeBucketIndex(hash, capacity);

            int64_t bSize = std::get<int64_t>(bucketSizes->get(static_cast<size_t>(bucket)));

            auto keyRow = std::get<std::shared_ptr<value::NativeArray>>(
                keyBuckets->get(static_cast<size_t>(bucket)));
            auto valRow = std::get<std::shared_ptr<value::NativeArray>>(
                valBuckets->get(static_cast<size_t>(bucket)));

            // Resize bucket row if needed
            if (bSize >= static_cast<int64_t>(keyRow->size()))
            {
                size_t newSize = keyRow->size() * 2;
                auto newKeyRow = std::make_shared<value::NativeArray>(newSize);
                auto newValRow = std::make_shared<value::NativeArray>(newSize);
                for (size_t k = 0; k < keyRow->size(); ++k)
                {
                    newKeyRow->set(k, keyRow->get(k));
                    newValRow->set(k, valRow->get(k));
                }
                keyBuckets->set(static_cast<size_t>(bucket), newKeyRow);
                valBuckets->set(static_cast<size_t>(bucket), newValRow);
                keyRow = newKeyRow;
                valRow = newValRow;
            }

            keyRow->set(static_cast<size_t>(bSize), key);
            valRow->set(static_cast<size_t>(bSize), val);
            bucketSizes->set(static_cast<size_t>(bucket), bSize + 1);
        }

        instance->setField("keyBuckets", keyBuckets);
        instance->setField("valueBuckets", valBuckets);
        instance->setField("bucketSizes", bucketSizes);
        instance->setField("capacity", capacity);
        instance->setField("count", count);

        return instance;
    }

    value::Value JsonDeserializer::deserializeHashSetCollection(
        const std::shared_ptr<JsonValue>& json)
    {
        auto classDef = environment->findClass("HashSet");
        if (!classDef)
            throw errors::RuntimeException("Unknown class 'HashSet' during JSON deserialization");

        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);

        if (json->hasProperty("__generics"))
            applyGenericBindings(instance, json->getProperty("__generics"));

        std::string elemType;
        const auto& bindings = instance->getGenericTypeBindings();
        auto tIt = bindings.find("T");
        if (tIt != bindings.end()) elemType = tIt->second;

        if (!json->hasProperty("elements"))
        {
            int64_t capacity = 16;
            int64_t bucketWidth = 4;
            auto buckets = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
            auto bucketSizes = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
            for (size_t i = 0; i < static_cast<size_t>(capacity); ++i)
            {
                buckets->set(i, std::make_shared<value::NativeArray>(static_cast<size_t>(bucketWidth)));
                bucketSizes->set(i, int64_t(0));
            }

            instance->setField("buckets", buckets);
            instance->setField("bucketSizes", bucketSizes);
            instance->setField("capacity", capacity);
            instance->setField("count", int64_t(0));
            return instance;
        }

        const auto& elements = json->getProperty("elements")->asArray();
        int64_t count = static_cast<int64_t>(elements.size());

        int64_t capacity = 16;
        while (count > capacity * 3 / 4) capacity *= 2;

        int64_t bucketWidth = 4;
        // Create jagged NativeArray structure to match VM runtime
        auto buckets = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
        auto bucketSizes = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
        for (size_t i = 0; i < static_cast<size_t>(capacity); ++i)
        {
            buckets->set(i, std::make_shared<value::NativeArray>(static_cast<size_t>(bucketWidth)));
            bucketSizes->set(i, int64_t(0));
        }

        for (size_t i = 0; i < elements.size(); ++i)
        {
            value::Value elem = elemType.empty()
                ? fromJsonValue(elements[i])
                : convertToFieldType(elements[i], elemType);

            int64_t hash = computeHashCode(elem);
            int64_t bucket = computeBucketIndex(hash, capacity);

            int64_t bSize = std::get<int64_t>(bucketSizes->get(static_cast<size_t>(bucket)));

            auto row = std::get<std::shared_ptr<value::NativeArray>>(
                buckets->get(static_cast<size_t>(bucket)));

            // Resize bucket row if needed
            if (bSize >= static_cast<int64_t>(row->size()))
            {
                size_t newSize = row->size() * 2;
                auto newRow = std::make_shared<value::NativeArray>(newSize);
                for (size_t k = 0; k < row->size(); ++k)
                    newRow->set(k, row->get(k));
                buckets->set(static_cast<size_t>(bucket), newRow);
                row = newRow;
            }

            row->set(static_cast<size_t>(bSize), elem);
            bucketSizes->set(static_cast<size_t>(bucket), bSize + 1);
        }

        instance->setField("buckets", buckets);
        instance->setField("bucketSizes", bucketSizes);
        instance->setField("capacity", capacity);
        instance->setField("count", count);

        return instance;
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
