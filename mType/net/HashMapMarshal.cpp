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

        // Mirrors HashMap.mt's getBucketIndex (and JsonDeserializer::computeBucketIndex).
        int64_t computeBucketIndex(int64_t hash, int64_t capacity)
        {
            if (hash < 0) { hash = -hash; if (hash < 0) hash += 1; }
            hash = hash * 1610612741;
            hash = hash + (hash / capacity);
            if (hash < 0) hash = -hash;
            return hash % capacity;
        }

        // Mirror HashCodeFunction's string hash (std::hash<std::string> & 0x7FFFFFFF)
        // so bucket placement matches what mType's HashMap.put would produce.
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

        auto keyBucketsVal = inst->getFieldValue("keyBuckets");
        auto valBucketsVal = inst->getFieldValue("valueBuckets");
        auto bucketSizesVal = inst->getFieldValue("bucketSizes");

        if (!value::isNativeArray(keyBucketsVal) ||
            !value::isNativeArray(valBucketsVal) ||
            !value::isNativeArray(bucketSizesVal))
        {
            return result;
        }

        auto keyBuckets = value::asNativeArray(keyBucketsVal);
        auto valBuckets = value::asNativeArray(valBucketsVal);
        auto bucketSizes = value::asNativeArray(bucketSizesVal);

        size_t cap = keyBuckets->size();
        for (size_t i = 0; i < cap; ++i)
        {
            auto sizeVal = bucketSizes->get(i);
            if (!value::isInt(sizeVal)) continue;
            int64_t bSize = value::asInt(sizeVal);

            auto kRowVal = keyBuckets->get(i);
            auto vRowVal = valBuckets->get(i);
            if (!value::isNativeArray(kRowVal) ||
                !value::isNativeArray(vRowVal))
                continue;

            auto kRow = value::asNativeArray(kRowVal);
            auto vRow = value::asNativeArray(vRowVal);
            for (int64_t j = 0; j < bSize; ++j)
            {
                std::string key = extractString(kRow->get(static_cast<size_t>(j)));
                std::string val = extractString(vRow->get(static_cast<size_t>(j)));
                result[key] = val;
            }
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
        int64_t capacity = 16;
        while (count > capacity * 3 / 4) capacity *= 2;
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

        for (const auto& [k, v] : map)
        {
            int64_t hash = stringHash(k);
            int64_t bucket = computeBucketIndex(hash, capacity);

            int64_t bSize = value::asInt(bucketSizes->get(static_cast<size_t>(bucket)));
            auto kRow = value::asNativeArray(keyBuckets->get(static_cast<size_t>(bucket)));
            auto vRow = value::asNativeArray(valBuckets->get(static_cast<size_t>(bucket)));

            if (bSize >= static_cast<int64_t>(kRow->size()))
            {
                size_t newSize = kRow->size() * 2;
                auto nk = std::make_shared<value::NativeArray>(newSize);
                auto nv = std::make_shared<value::NativeArray>(newSize);
                for (size_t i = 0; i < kRow->size(); ++i)
                {
                    nk->set(i, kRow->get(i));
                    nv->set(i, vRow->get(i));
                }
                keyBuckets->set(static_cast<size_t>(bucket), nk);
                valBuckets->set(static_cast<size_t>(bucket), nv);
                kRow = nk;
                vRow = nv;
            }

            kRow->set(static_cast<size_t>(bSize), boxString(env, k));
            vRow->set(static_cast<size_t>(bSize), boxString(env, v));
            bucketSizes->set(static_cast<size_t>(bucket), bSize + 1);
        }

        instance->setField("keyBuckets", keyBuckets);
        instance->setField("valueBuckets", valBuckets);
        instance->setField("bucketSizes", bucketSizes);
        instance->setField("capacity", capacity);
        instance->setField("count", count);

        return instance;
    }
}
