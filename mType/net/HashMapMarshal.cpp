#include "HashMapMarshal.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../value/NativeArray.hpp"
#include "../value/ObjectInstancePool.hpp"
#include "../value/ValueShim.hpp"
#include "../errors/RuntimeException.hpp"

namespace net
{
    namespace
    {
        // Named `extractString` (not `asString`) to avoid ambiguity with the
        // ValueShim's `value::asString`, which this file now depends on.
        std::string extractString(const value::Value& v)
        {
            if (value::isString(v))
                return value::asString(v);
            if (value::isInternedString(v))
                return value::asInternedString(v).getString();
            // Boxed String instance: pull the inner `value` field.
            if (value::isObject(v))
            {
                auto inst = value::asObject(v);
                if (inst)
                {
                    auto inner = inst->getFieldValue("value");
                    if (value::isString(inner))
                        return value::asString(inner);
                    if (value::isInternedString(inner))
                        return value::asInternedString(inner).getString();
                }
            }
            return "";
        }

        // Wrap a std::string as an mType String boxed instance.
        value::Value boxString(std::shared_ptr<environment::Environment> env, const std::string& s)
        {
            auto stringClass = env ? env->findClass("String") : nullptr;
            if (!stringClass)
            {
                return s;  // fall back to raw string if String class not loaded
            }
            auto inst = value::ObjectInstancePool::getInstance().acquire(stringClass);
            inst->setField("value", s);
            return inst;
        }

        // Mirrors HashMap.mt's hash-to-slot mapping after MYT-258 Phase 3:
        // open-addressing on flat arrays with `(rawHash * MAGIC ^ shift) & (cap - 1)`.
        int64_t computeSlotIndex(int64_t hash, int64_t capacity)
        {
            int64_t mixed = hash * 1610612741;
            return (mixed ^ (mixed >> 16)) & (capacity - 1);
        }

        // Mirror BuiltinNatives::hashCode_fn string hash (std::hash<std::string> & 0x7FFFFFFF)
        // so slot placement matches what mType's HashMap.put would produce.
        int64_t stringHash(const std::string& s)
        {
            std::hash<std::string> hasher;
            return static_cast<int64_t>(hasher(s) & 0x7FFFFFFF);
        }
    }

    std::unordered_map<std::string, std::string> hashMapToStdMap(const value::Value& v)
    {
        std::unordered_map<std::string, std::string> result;
        if (!value::isObject(v))
            return result;

        auto inst = value::asObject(v);
        if (!inst) return result;

        if (auto* storage = inst->getSpecializedCollection())
        {
            auto keys = storage->materializeKeys(nullptr);
            auto values = storage->materializeValues();
            for (size_t i = 0; i < storage->size(); ++i)
            {
                result[extractString(keys->get(i))] = extractString(values->get(i));
            }
            return result;
        }

        auto keysVal = inst->getFieldValue("keys");
        auto valsVal = inst->getFieldValue("values");

        if (!value::isNativeArray(keysVal) || !value::isNativeArray(valsVal))
            return result;

        auto keys = value::asNativeArray(keysVal);
        auto values = value::asNativeArray(valsVal);

        size_t cap = keys->size();
        for (size_t i = 0; i < cap; ++i)
        {
            auto kVal = keys->get(i);
            if (value::isNullType(kVal)) continue;

            std::string key = extractString(kVal);
            std::string val = extractString(values->get(i));
            result[key] = val;
        }
        return result;
    }

    value::Value stdMapToHashMap(std::shared_ptr<environment::Environment> env,
                                 const std::unordered_map<std::string, std::string>& map)
    {
        if (!env)
            throw errors::RuntimeException("stdMapToHashMap: null environment");

        auto classDef = env->findClass("HashMap");
        if (!classDef)
            throw errors::RuntimeException("HashMap class not loaded; import lib/collections/HashMap.mt");

        auto instance = value::ObjectInstancePool::getInstance().acquire(classDef);

        int64_t count = static_cast<int64_t>(map.size());
        int64_t capacity = 32;
        while (count > capacity * 3 / 4) capacity *= 2;

        auto keys = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
        auto values = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
        auto hashes = std::make_shared<value::NativeArray>(static_cast<size_t>(capacity));
        // 1-arg NativeArray ctor uses VOID type — null-init so linear probe finds empty slots.
        for (size_t i = 0; i < static_cast<size_t>(capacity); ++i)
        {
            keys->set(i, nullptr);
            values->set(i, nullptr);
        }

        const int64_t mask = capacity - 1;
        for (const auto& [k, v] : map)
        {
            int64_t rawHash = stringHash(k);
            int64_t idx = computeSlotIndex(rawHash, capacity);

            // Linear probe to next empty slot.
            while (!value::isNullType(keys->get(static_cast<size_t>(idx))))
                idx = (idx + 1) & mask;

            keys->set(static_cast<size_t>(idx), boxString(env, k));
            values->set(static_cast<size_t>(idx), boxString(env, v));
            hashes->set(static_cast<size_t>(idx), rawHash);
        }

        instance->setField("keys", keys);
        instance->setField("values", values);
        instance->setField("hashes", hashes);
        instance->setField("capacity", capacity);
        instance->setField("count", count);

        return instance;
    }
}
