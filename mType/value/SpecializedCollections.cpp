#include "SpecializedCollections.hpp"
#include "ValueShim.hpp"
#include "HashUtils.hpp"
#include "ValueObject.hpp"
#include "../gc/WriteBarrier.hpp"
#include "../environment/registry/ClassDefinition.hpp"
#include "../value/ObjectInstance.hpp"
#include <functional>
// Key extraction, boxing, and materialization live in
// SpecializedCollectionsBoxing.cpp to keep this file focused on the
// storage core (probe/insert/erase/resize, hash, equality).

namespace value
{
    namespace
    {
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

        bool storedValuesEqual(const Value& stored, const Value& candidate)
        {
            if (isNullType(stored) || isNullType(candidate))
            {
                return isNullType(stored) && isNullType(candidate);
            }

            if (isValueObject(stored) && isValueObject(candidate))
            {
                auto lhs = asValueObject(stored);
                auto rhs = asValueObject(candidate);
                if (!lhs || !rhs) return lhs == rhs;
                return lhs->equals(*rhs);
            }

            if (isAnyObject(stored) && isAnyObject(candidate))
            {
                auto* lhs = asObjectInstanceRaw(stored);
                auto* rhs = asObjectInstanceRaw(candidate);
                if (!lhs || !rhs) return lhs == rhs;
                return lhs->contentEquals(*rhs);
            }

            return stored == candidate;
        }
    }

    SpecializedCollectionStorage::SpecializedCollectionStorage(
        Kind kind,
        PrimitiveTypeTag keyTag,
        size_t initialCapacity)
        : kind_(kind),
          mode_(Mode::PRIMITIVE),
          keyTag_(keyTag),
          entries_(normalizeCapacity(initialCapacity))
    {
    }

    SpecializedCollectionStorage::SpecializedCollectionStorage(
        Kind kind,
        ShapeDescriptor shape,
        size_t initialCapacity)
        : kind_(kind),
          mode_(Mode::SHAPE),
          keyTag_(PrimitiveTypeTag::NONE),
          shape_(std::move(shape)),
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

    bool SpecializedCollectionStorage::isSpecializableShape(
        const runtimeTypes::klass::ClassDefinition* classDef)
    {
        if (!classDef) return false;
        if (classDef->isAbstract() || classDef->isValueClass() || classDef->isGeneric()) return false;
        // Phase 1: no in-program parent. MYT-274's STRUCT_*_INT fast emit
        // takes the same gate; storage and synth methods stay in lockstep.
        if (classDef->hasParentClass())
        {
            const std::string& parentName = classDef->getParentClassName();
            if (!parentName.empty() && parentName != "Object") return false;
        }

        const auto& fields = classDef->getInstanceFields();
        if (fields.empty()) return false;
        size_t intFieldCount = 0;
        for (const auto& [name, fieldDef] : fields)
        {
            if (!fieldDef || fieldDef->isStatic()) continue;
            if (fieldDef->getType() != ValueType::INT) return false;
            ++intFieldCount;
        }
        if (intFieldCount == 0) return false;
        if (intFieldCount > kMaxShapeFields) return false;

        // Require synth-only equals/hashCode. findInstanceMethod walks user
        // methods first, falling through to synthetic; if we find a non-
        // synthetic match, the user overrode the contract and the storage's
        // field-wise compare would silently disagree.
        auto eq = classDef->findInstanceMethod("equals", 1);
        auto hc = classDef->findInstanceMethod("hashCode", 0);
        if (!eq || !hc) return false;
        if (!eq->isSynthetic() || !hc->isSynthetic()) return false;
        return true;
    }

    ShapeDescriptor SpecializedCollectionStorage::buildShapeDescriptor(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef)
    {
        ShapeDescriptor desc;
        if (!isSpecializableShape(classDef.get())) return desc;

        // Slot indices 0..n-1 in declaration order match
        // ClassRegistrar's fieldIndexMap and the slots emitted by the
        // STRUCT_HASH_INT / STRUCT_EQ_INT fast-body path
        // (MethodCompilerHelper::tryEmitStructuralFastBody).
        const size_t n = classDef->getIndexedFieldCount();
        desc.fields.reserve(n);
        for (size_t i = 0; i < n && i < kMaxShapeFields; ++i)
        {
            ShapeFieldDescriptor f;
            f.fieldIndex = i;
            f.type = ValueType::INT;
            desc.fields.push_back(f);
        }
        desc.classDef = std::move(classDef);
        return desc;
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

    int64_t SpecializedCollectionStorage::rawHash(const Key& key) const
    {
        if (mode_ == Mode::SHAPE)
        {
            // Horner-31, h0 = 1. Mirrors jit_struct_hash_int and the synth
            // hashCode body so storage and user-visible hashCode agree.
            int64_t h = 1;
            for (uint8_t i = 0; i < key.shapeFieldCount; ++i)
            {
                h = h * 31 + key.shapeInts[i];
            }
            return maskedHash(static_cast<size_t>(h));
        }
        switch (key.tag)
        {
        case PrimitiveTypeTag::INT:
            return ::value::hashutils::intHash(key.intValue);
        case PrimitiveTypeTag::FLOAT:
            return ::value::hashutils::floatHash(key.floatValue);
        case PrimitiveTypeTag::BOOL:
            return ::value::hashutils::boolHash(key.boolValue);
        case PrimitiveTypeTag::STRING:
            return ::value::hashutils::stringHash(key.stringValue);
        default:
            return 0;
        }
    }

    bool SpecializedCollectionStorage::keysEqual(const Key& a, const Key& b) const
    {
        if (mode_ == Mode::SHAPE)
        {
            if (a.shapeFieldCount != b.shapeFieldCount) return false;
            for (uint8_t i = 0; i < a.shapeFieldCount; ++i)
            {
                if (a.shapeInts[i] != b.shapeInts[i]) return false;
            }
            return true;
        }
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
        size_t idx = static_cast<size_t>(hash & static_cast<int64_t>(mask));

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
            if (storedValuesEqual(entry.value, value)) return true;
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
