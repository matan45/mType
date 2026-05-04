#include "SpecializedCollections.hpp"
#include "BoolCache.hpp"
#include "IntegerCache.hpp"
#include "NativeArray.hpp"
#include "StringPool.hpp"
#include "ValueObject.hpp"
#include "ValueShim.hpp"
#include "../environment/Environment.hpp"
#include "../gc/WriteBarrier.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include <algorithm>
#include <functional>

namespace value
{
    namespace
    {
        constexpr int64_t kHashMultiplier = 1610612741;

        size_t normalizeCapacity(size_t requested)
        {
            size_t cap = 4;
            while (cap < requested) cap <<= 1;
            return cap;
        }

        int64_t maskedHash(size_t hash)
        {
            return static_cast<int64_t>(hash & 0x7FFFFFFF);
        }

        Value primitiveFieldZero(const Value& value)
        {
            if (isValueObject(value))
            {
                auto obj = asValueObject(value);
                if (obj) return obj->getFieldByIndex(0);
            }
            if (isAnyObject(value))
            {
                auto* obj = asObjectInstanceRaw(value);
                if (obj) return obj->getFieldByIndex(0);
            }
            return std::monostate{};
        }
    }

    SpecializedCollectionStorage::SpecializedCollectionStorage(
        Kind kind,
        PrimitiveTypeTag keyTag,
        size_t initialCapacity)
        : kind_(kind),
          keyTag_(keyTag),
          entries_(normalizeCapacity(initialCapacity))
    {
    }

    bool SpecializedCollectionStorage::isSpecializableKeyTag(PrimitiveTypeTag tag) noexcept
    {
        return tag == PrimitiveTypeTag::INT ||
               tag == PrimitiveTypeTag::FLOAT ||
               tag == PrimitiveTypeTag::BOOL ||
               tag == PrimitiveTypeTag::STRING;
    }

    void SpecializedCollectionStorage::clear()
    {
        for (auto& entry : entries_)
        {
            entry.occupied = false;
            entry.value = std::monostate{};
            entry.key = Key{};
        }
        count_ = 0;
    }

    bool SpecializedCollectionStorage::extractKey(const Value& value, Key& out) const
    {
        out = Key{};
        out.tag = keyTag_;

        Value source = value;
        if (isValueObject(value))
        {
            auto obj = asValueObject(value);
            if (!obj || obj->getPrimitiveTag() != keyTag_) return false;
            source = obj->getFieldByIndex(0);
        }
        else if (isAnyObject(value))
        {
            auto* obj = asObjectInstanceRaw(value);
            if (!obj || obj->getPrimitiveTag() != keyTag_) return false;
            source = obj->getFieldByIndex(0);
        }

        switch (keyTag_)
        {
        case PrimitiveTypeTag::INT:
            if (!isInt(source)) return false;
            out.intValue = asInt(source);
            return true;
        case PrimitiveTypeTag::FLOAT:
            if (!isFloat(source)) return false;
            out.floatValue = asFloat(source);
            return true;
        case PrimitiveTypeTag::BOOL:
            if (!isBool(source)) return false;
            out.boolValue = asBool(source);
            return true;
        case PrimitiveTypeTag::STRING:
            if (isString(source))
            {
                out.stringValue = asString(source);
                return true;
            }
            if (isInternedString(source))
            {
                out.stringValue = asInternedString(source).getString();
                return true;
            }
            return false;
        default:
            return false;
        }
    }

    int64_t SpecializedCollectionStorage::rawHash(const Key& key) const
    {
        switch (key.tag)
        {
        case PrimitiveTypeTag::INT:
            return maskedHash(std::hash<int64_t>{}(key.intValue));
        case PrimitiveTypeTag::FLOAT:
            return maskedHash(std::hash<double>{}(key.floatValue));
        case PrimitiveTypeTag::BOOL:
            return key.boolValue ? 1231 : 1237;
        case PrimitiveTypeTag::STRING:
            return maskedHash(std::hash<std::string>{}(key.stringValue));
        default:
            return 0;
        }
    }

    bool SpecializedCollectionStorage::keysEqual(const Key& a, const Key& b) const
    {
        if (a.tag != b.tag) return false;
        switch (a.tag)
        {
        case PrimitiveTypeTag::INT:
            return a.intValue == b.intValue;
        case PrimitiveTypeTag::FLOAT:
            return a.floatValue == b.floatValue;
        case PrimitiveTypeTag::BOOL:
            return a.boolValue == b.boolValue;
        case PrimitiveTypeTag::STRING:
            return a.stringValue == b.stringValue;
        default:
            return false;
        }
    }

    bool SpecializedCollectionStorage::findSlot(const Key& key, size_t& slot, bool& found) const
    {
        if (entries_.empty()) return false;

        const size_t mask = entries_.size() - 1;
        const int64_t hash = rawHash(key);
        const int64_t mixed = hash * kHashMultiplier;
        size_t idx = static_cast<size_t>((mixed ^ (mixed >> 16)) & static_cast<int64_t>(mask));

        for (size_t probes = 0; probes < entries_.size(); ++probes)
        {
            const auto& entry = entries_[idx];
            if (!entry.occupied)
            {
                slot = idx;
                found = false;
                return true;
            }
            if (keysEqual(entry.key, key))
            {
                slot = idx;
                found = true;
                return true;
            }
            idx = (idx + 1) & mask;
        }
        return false;
    }

    void SpecializedCollectionStorage::resize(size_t newCapacity)
    {
        auto oldEntries = std::move(entries_);
        entries_.clear();
        entries_.resize(normalizeCapacity(newCapacity));
        count_ = 0;
        for (auto& entry : oldEntries)
        {
            if (entry.occupied)
            {
                insertKnownNew(std::move(entry.key), std::move(entry.value));
            }
        }
    }

    void SpecializedCollectionStorage::insertKnownNew(Key key, Value value)
    {
        size_t slot = 0;
        bool found = false;
        if (!findSlot(key, slot, found) || found) return;

        entries_[slot].occupied = true;
        entries_[slot].key = std::move(key);
        entries_[slot].value = std::move(value);
        ++count_;
    }

    void SpecializedCollectionStorage::eraseSlot(size_t slot)
    {
        const size_t mask = entries_.size() - 1;
        entries_[slot].occupied = false;
        entries_[slot].value = std::monostate{};
        entries_[slot].key = Key{};
        --count_;

        size_t probe = (slot + 1) & mask;
        while (entries_[probe].occupied)
        {
            Key key = std::move(entries_[probe].key);
            Value value = std::move(entries_[probe].value);
            entries_[probe].occupied = false;
            entries_[probe].value = std::monostate{};
            entries_[probe].key = Key{};
            --count_;
            insertKnownNew(std::move(key), std::move(value));
            probe = (probe + 1) & mask;
        }
    }

    bool SpecializedCollectionStorage::put(const Value& keyValue, const Value& value, Value& oldValue)
    {
        Key key;
        if (kind_ != Kind::MAP || !extractKey(keyValue, key)) return false;

        if ((count_ + 1) > entries_.size() * 3 / 4)
        {
            resize(entries_.size() * 2);
        }

        size_t slot = 0;
        bool found = false;
        if (!findSlot(key, slot, found)) return false;

        if (found)
        {
            oldValue = entries_[slot].value;
            entries_[slot].value = value;
            return true;
        }

        entries_[slot].occupied = true;
        entries_[slot].key = std::move(key);
        entries_[slot].value = value;
        ++count_;
        oldValue = nullptr;
        return true;
    }

    bool SpecializedCollectionStorage::get(const Value& keyValue, Value& out) const
    {
        Key key;
        if (kind_ != Kind::MAP || !extractKey(keyValue, key)) return false;

        size_t slot = 0;
        bool found = false;
        if (!findSlot(key, slot, found) || !found)
        {
            out = nullptr;
            return true;
        }
        out = entries_[slot].value;
        return true;
    }

    bool SpecializedCollectionStorage::containsKey(const Value& keyValue) const
    {
        Key key;
        if (!extractKey(keyValue, key)) return false;

        size_t slot = 0;
        bool found = false;
        return findSlot(key, slot, found) && found;
    }

    bool SpecializedCollectionStorage::remove(const Value& keyValue)
    {
        Key key;
        if (!extractKey(keyValue, key)) return false;

        size_t slot = 0;
        bool found = false;
        if (!findSlot(key, slot, found) || !found) return false;

        eraseSlot(slot);
        return true;
    }

    bool SpecializedCollectionStorage::containsStoredValue(const Value& value) const
    {
        if (kind_ != Kind::MAP) return false;
        for (const auto& entry : entries_)
        {
            if (!entry.occupied) continue;
            if (isNullType(entry.value) && isNullType(value)) return true;
            if (!isNullType(entry.value) && entry.value == value) return true;
        }
        return false;
    }

    bool SpecializedCollectionStorage::add(const Value& item)
    {
        if (kind_ != Kind::SET) return false;
        Key key;
        if (!extractKey(item, key)) return false;

        if ((count_ + 1) > entries_.size() * 3 / 4)
        {
            resize(entries_.size() * 2);
        }

        size_t slot = 0;
        bool found = false;
        if (!findSlot(key, slot, found)) return false;
        if (found) return false;

        entries_[slot].occupied = true;
        entries_[slot].key = std::move(key);
        entries_[slot].value = std::monostate{};
        ++count_;
        return true;
    }

    bool SpecializedCollectionStorage::contains(const Value& item) const
    {
        if (kind_ != Kind::SET) return false;
        return containsKey(item);
    }

    Value SpecializedCollectionStorage::boxKey(const Key& key, environment::Environment* env) const
    {
        if (!env)
        {
            switch (key.tag)
            {
            case PrimitiveTypeTag::INT: return key.intValue;
            case PrimitiveTypeTag::FLOAT: return key.floatValue;
            case PrimitiveTypeTag::BOOL: return key.boolValue;
            case PrimitiveTypeTag::STRING: return Value(StringPool::getInstance().intern(key.stringValue));
            default: return nullptr;
            }
        }

        const char* className = nullptr;
        switch (key.tag)
        {
        case PrimitiveTypeTag::INT: className = "Int"; break;
        case PrimitiveTypeTag::FLOAT: className = "Float"; break;
        case PrimitiveTypeTag::BOOL: className = "Bool"; break;
        case PrimitiveTypeTag::STRING: className = "String"; break;
        default: return nullptr;
        }

        auto classDef = env->findClass(className);
        if (!classDef) return nullptr;

        if (key.tag == PrimitiveTypeTag::INT)
        {
            if (auto cached = IntegerCache::getInt(key.intValue, classDef))
            {
                return cached;
            }
        }
        if (key.tag == PrimitiveTypeTag::BOOL)
        {
            if (auto cached = BoolCache::getBool(key.boolValue, classDef))
            {
                return cached;
            }
        }

        auto boxed = std::make_shared<ValueObject>(classDef);
        switch (key.tag)
        {
        case PrimitiveTypeTag::INT:
            boxed->setFieldByIndex(0, key.intValue);
            break;
        case PrimitiveTypeTag::FLOAT:
            boxed->setFieldByIndex(0, key.floatValue);
            break;
        case PrimitiveTypeTag::BOOL:
            boxed->setFieldByIndex(0, key.boolValue);
            break;
        case PrimitiveTypeTag::STRING:
            boxed->setFieldByIndex(0, Value(StringPool::getInstance().intern(key.stringValue)));
            break;
        default:
            return nullptr;
        }
        return boxed;
    }

    std::shared_ptr<NativeArray> SpecializedCollectionStorage::materializeKeys(environment::Environment* env) const
    {
        auto array = std::make_shared<NativeArray>(count_);
        size_t index = 0;
        for (const auto& entry : entries_)
        {
            if (!entry.occupied) continue;
            array->set(index++, boxKey(entry.key, env));
        }
        return array;
    }

    std::shared_ptr<NativeArray> SpecializedCollectionStorage::materializeValues() const
    {
        auto array = std::make_shared<NativeArray>(count_);
        size_t index = 0;
        for (const auto& entry : entries_)
        {
            if (!entry.occupied) continue;
            array->set(index++, entry.value);
        }
        return array;
    }

    void SpecializedCollectionStorage::visitReferences(const std::function<void(void*)>& callback) const
    {
        if (kind_ != Kind::MAP) return;
        for (const auto& entry : entries_)
        {
            if (!entry.occupied) continue;
            void* ptr = gc::extractPointer(entry.value);
            if (ptr) callback(ptr);
        }
    }

    bool SpecializedCollectionStorage::isSpecializedMethod(const std::string& methodName, size_t argCount) const
    {
        if (kind_ == Kind::MAP)
        {
            return (methodName == "put" && argCount == 2) ||
                   (methodName == "get" && argCount == 1) ||
                   (methodName == "containsKey" && argCount == 1) ||
                   (methodName == "remove" && argCount == 1) ||
                   (methodName == "size" && argCount == 0) ||
                   (methodName == "empty" && argCount == 0) ||
                   (methodName == "clear" && argCount == 0) ||
                   (methodName == "getKeys" && argCount == 0) ||
                   (methodName == "getValues" && argCount == 0) ||
                   (methodName == "toArray" && argCount == 0) ||
                   (methodName == "containsValue" && argCount == 1);
        }

        return (methodName == "add" && argCount == 1) ||
               (methodName == "contains" && argCount == 1) ||
               (methodName == "remove" && argCount == 1) ||
               (methodName == "size" && argCount == 0) ||
               (methodName == "empty" && argCount == 0) ||
               (methodName == "clear" && argCount == 0) ||
               (methodName == "toArray" && argCount == 0);
    }
}
