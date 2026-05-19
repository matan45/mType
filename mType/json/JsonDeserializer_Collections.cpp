#include "JsonDeserializer.hpp"
#include "../value/HashUtils.hpp"
#include <cstddef>
#include <cstdint>
#include "../value/ObjectInstance.hpp"
#include "../environment/registry/ClassDefinition.hpp"
#include "../value/NativeArray.hpp"
#include "../value/ObjectInstancePool.hpp"
#include "../value/ValueShim.hpp"
#include "../errors/RuntimeException.hpp"

namespace
{
    // Routes through value::hashutils so the runtime native, IC/JIT fast
    // leaves, and storage all share one source of truth for primitive hash.
    int64_t hashPrimitive(const value::Value& val)
    {
        if (value::isInt(val))
            return ::value::hashutils::intHash(value::asInt(val));
        if (value::isFloat(val))
            return ::value::hashutils::floatHash(value::asFloat(val));
        if (value::isBool(val))
            return ::value::hashutils::boolHash(value::asBool(val));
        // MYT-317: SSO-aware. Folds all three string forms into one hash.
        if (value::isAnyString(val))
            return ::value::hashutils::stringHash(value::asStringView(val));
        return 0;
    }

    value::PrimitiveTypeTag specializableTypeNameToTag(const std::string& typeName)
    {
        if (typeName == "Int" || typeName == "int") return value::PrimitiveTypeTag::INT;
        if (typeName == "Float" || typeName == "float") return value::PrimitiveTypeTag::FLOAT;
        if (typeName == "Bool" || typeName == "bool") return value::PrimitiveTypeTag::BOOL;
        if (typeName == "String" || typeName == "string") return value::PrimitiveTypeTag::STRING;
        return value::PrimitiveTypeTag::NONE;
    }
}

namespace json
{
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

        auto instance = value::ObjectInstancePool::getInstance().acquire(classDef);

        if (json->hasProperty("__generics"))
            applyGenericBindings(instance, json->getProperty("__generics"));

        if (!json->hasProperty("elements"))
            return instance;

        const auto& elements = json->getProperty("elements")->asArray();
        int64_t count = static_cast<int64_t>(elements.size());

        // Resolve element type from generic binding.
        std::string elemType;
        const auto& bindings = instance->getGenericTypeBindings();
        auto tIt = bindings.find("T");
        if (tIt != bindings.end())
            elemType = tIt->second;

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

        auto instance = value::ObjectInstancePool::getInstance().acquire(classDef);

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

        std::shared_ptr<runtimeTypes::klass::ObjectInstance> headNode;
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> prevNode;

        for (size_t i = 0; i < elements.size(); ++i)
        {
            auto node = value::ObjectInstancePool::getInstance().acquire(nodeDef);
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

    // Mirrors the native hashCode() built-in (environment/registry/builtin/BuiltinNatives.cpp).
    // String values use deterministic FNV-1a; other primitives keep the runtime hash arms.
    // WARNING: if BuiltinNatives.cpp's hashCode_fn changes, this must be updated to match.
    int64_t JsonDeserializer::computeHashCode(const value::Value& val)
    {
        if (value::isInt(val) || value::isFloat(val) || value::isBool(val) ||
            value::isString(val) || value::isInternedString(val))
        {
            return hashPrimitive(val);
        }

        // Object wrapper: pull the `value` field and hash its primitive payload.
        if (value::isObject(val))
        {
            auto obj = value::asObject(val);
            if (!obj) return 0;
            value::Value innerVal = obj->getFieldValue("value");
            if (value::isVoid(innerVal)) return 0;
            return hashPrimitive(innerVal);
        }

        // Value-object wrapper: same idea, via ValueObject::getFieldValue.
        if (value::isValueObject(val))
        {
            auto vo = value::asValueObject(val);
            if (!vo) return 0;
            value::Value inner = vo->getFieldValue("value");
            return hashPrimitive(inner);
        }

        return 0;
    }

    // Mirrors HashMap.mt/HashSet.mt hash-to-slot mapping.
    // WARNING: if HashMap.mt's getBucketIndex() changes, this must be updated to match.
    // Capacity is always a power of two.
    int64_t JsonDeserializer::computeBucketIndex(int64_t hash, int64_t capacity)
    {
        return hash & (capacity - 1);
    }

    value::Value JsonDeserializer::deserializeHashMapCollection(
        const std::shared_ptr<JsonValue>& json)
    {
        auto classDef = environment->findClass("HashMap");
        if (!classDef)
            throw errors::RuntimeException("Unknown class 'HashMap' during JSON deserialization");

        auto instance = value::ObjectInstancePool::getInstance().acquire(classDef);

        if (json->hasProperty("__generics"))
            applyGenericBindings(instance, json->getProperty("__generics"));

        std::string keyType, valType;
        const auto& bindings = instance->getGenericTypeBindings();
        auto kIt = bindings.find("K");
        auto vIt = bindings.find("V");
        if (kIt != bindings.end()) keyType = kIt->second;
        if (vIt != bindings.end()) valType = vIt->second;

        auto specializedKeyTag = specializableTypeNameToTag(keyType);
        if (value::SpecializedCollectionStorage::isSpecializableKeyTag(specializedKeyTag))
        {
            size_t initialCapacity = 32;
            if (json->hasProperty("entries"))
            {
                const auto& entries = json->getProperty("entries")->asArray();
                while (entries.size() > initialCapacity * 3 / 4) initialCapacity *= 2;
                instance->attachSpecializedCollection(
                    value::SpecializedCollectionStorage::Kind::MAP,
                    specializedKeyTag,
                    initialCapacity);
                auto* storage = instance->getSpecializedCollection();
                for (size_t e = 0; e < entries.size(); ++e)
                {
                    auto entry = entries[e];
                    value::Value key = keyType.empty()
                        ? fromJsonValue(entry->getProperty("key"))
                        : convertToFieldType(entry->getProperty("key"), keyType);
                    value::Value val = valType.empty()
                        ? fromJsonValue(entry->getProperty("value"))
                        : convertToFieldType(entry->getProperty("value"), valType);
                    value::Value oldValue;
                    storage->put(key, val, oldValue);
                }
            }
            else
            {
                instance->attachSpecializedCollection(
                    value::SpecializedCollectionStorage::Kind::MAP,
                    specializedKeyTag,
                    initialCapacity);
            }
            return instance;
        }

        if (!json->hasProperty("entries"))
        {
            // Phase 3 layout: flat 1D NativeArrays, length = capacity.
            // keys[i] == null marks empty slot.
            int64_t capacity = 32;
            auto keys = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
            auto values = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
            auto hashes = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
            // 1-arg NativeArray ctor uses VOID type — slots default to monostate,
            // not NULL_TYPE. Explicitly null-init so the linear probe terminates.
            for (size_t i = 0; i < static_cast<size_t>(capacity); ++i)
            {
                keys->set(i, nullptr);
                values->set(i, nullptr);
            }

            instance->setField("keys", keys);
            instance->setField("values", values);
            instance->setField("hashes", hashes);
            instance->setField("capacity", capacity);
            instance->setField("count", int64_t(0));
            return instance;
        }

        const auto& entries = json->getProperty("entries")->asArray();
        int64_t count = static_cast<int64_t>(entries.size());

        int64_t capacity = 32;
        while (count > capacity * 3 / 4) capacity *= 2;

        auto keys = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
        auto values = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
        auto hashes = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
        // Null-init: VOID-typed arrays don't default to NULL_TYPE, so the
        // linear probe needs explicit nulls to find empty slots.
        for (size_t i = 0; i < static_cast<size_t>(capacity); ++i)
        {
            keys->set(i, nullptr);
            values->set(i, nullptr);
        }

        const int64_t mask = capacity - 1;
        for (size_t e = 0; e < entries.size(); ++e)
        {
            auto entry = entries[e];
            value::Value key = keyType.empty()
                ? fromJsonValue(entry->getProperty("key"))
                : convertToFieldType(entry->getProperty("key"), keyType);
            value::Value val = valType.empty()
                ? fromJsonValue(entry->getProperty("value"))
                : convertToFieldType(entry->getProperty("value"), valType);

            int64_t rawHash = computeHashCode(key);
            int64_t idx = rawHash & mask;

            while (!value::isNullType(keys->get(static_cast<size_t>(idx))))
                idx = (idx + 1) & mask;

            keys->set(static_cast<size_t>(idx), key);
            values->set(static_cast<size_t>(idx), val);
            hashes->set(static_cast<size_t>(idx), rawHash);
        }

        instance->setField("keys", keys);
        instance->setField("values", values);
        instance->setField("hashes", hashes);
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

        auto instance = value::ObjectInstancePool::getInstance().acquire(classDef);

        if (json->hasProperty("__generics"))
            applyGenericBindings(instance, json->getProperty("__generics"));

        std::string elemType;
        const auto& bindings = instance->getGenericTypeBindings();
        auto tIt = bindings.find("T");
        if (tIt != bindings.end()) elemType = tIt->second;

        auto specializedElemTag = specializableTypeNameToTag(elemType);
        if (value::SpecializedCollectionStorage::isSpecializableKeyTag(specializedElemTag))
        {
            size_t initialCapacity = 32;
            if (json->hasProperty("elements"))
            {
                const auto& elementsJson = json->getProperty("elements")->asArray();
                while (elementsJson.size() > initialCapacity * 3 / 4) initialCapacity *= 2;
                instance->attachSpecializedCollection(
                    value::SpecializedCollectionStorage::Kind::SET,
                    specializedElemTag,
                    initialCapacity);
                auto* storage = instance->getSpecializedCollection();
                for (size_t e = 0; e < elementsJson.size(); ++e)
                {
                    value::Value elem = elemType.empty()
                        ? fromJsonValue(elementsJson[e])
                        : convertToFieldType(elementsJson[e], elemType);
                    storage->add(elem);
                }
            }
            else
            {
                instance->attachSpecializedCollection(
                    value::SpecializedCollectionStorage::Kind::SET,
                    specializedElemTag,
                    initialCapacity);
            }
            return instance;
        }

        if (!json->hasProperty("elements"))
        {
            // Phase 3 layout: flat 1D NativeArrays.
            int64_t capacity = 32;
            auto elements = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
            auto hashes = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
            // Null-init for empty-slot detection (VOID array slots default to monostate).
            for (size_t i = 0; i < static_cast<size_t>(capacity); ++i)
                elements->set(i, nullptr);

            instance->setField("elements", elements);
            instance->setField("hashes", hashes);
            instance->setField("capacity", capacity);
            instance->setField("count", int64_t(0));
            return instance;
        }

        const auto& elementsJson = json->getProperty("elements")->asArray();
        int64_t count = static_cast<int64_t>(elementsJson.size());

        int64_t capacity = 32;
        while (count > capacity * 3 / 4) capacity *= 2;

        auto elements = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
        auto hashes = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
        // Null-init for empty-slot detection.
        for (size_t i = 0; i < static_cast<size_t>(capacity); ++i)
            elements->set(i, nullptr);

        const int64_t mask = capacity - 1;
        for (size_t e = 0; e < elementsJson.size(); ++e)
        {
            value::Value elem = elemType.empty()
                ? fromJsonValue(elementsJson[e])
                : convertToFieldType(elementsJson[e], elemType);

            int64_t rawHash = computeHashCode(elem);
            int64_t idx = rawHash & mask;

            while (!value::isNullType(elements->get(static_cast<size_t>(idx))))
                idx = (idx + 1) & mask;

            elements->set(static_cast<size_t>(idx), elem);
            hashes->set(static_cast<size_t>(idx), rawHash);
        }

        instance->setField("elements", elements);
        instance->setField("hashes", hashes);
        instance->setField("capacity", capacity);
        instance->setField("count", count);

        return instance;
    }
}
