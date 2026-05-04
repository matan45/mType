#pragma once

#include "PrimitiveTypeTag.hpp"
#include "ValueType.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace environment
{
    class Environment;
}

namespace runtimeTypes::klass
{
    class ClassDefinition;
}

namespace value
{
    class NativeArray;

    inline constexpr size_t kMaxShapeFields = 4;

    struct ShapeFieldDescriptor
    {
        size_t fieldIndex = 0;
        ValueType type = ValueType::INT;
    };

    struct ShapeDescriptor
    {
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef;
        std::vector<ShapeFieldDescriptor> fields;
        bool empty() const noexcept { return fields.empty(); }
    };

    class SpecializedCollectionStorage
    {
    public:
        enum class Kind : uint8_t
        {
            MAP,
            SET
        };

        enum class Mode : uint8_t
        {
            PRIMITIVE,
            SHAPE
        };

        struct Key
        {
            PrimitiveTypeTag tag = PrimitiveTypeTag::NONE;
            int64_t intValue = 0;
            double floatValue = 0.0;
            bool boolValue = false;
            std::string stringValue;
            std::array<int64_t, kMaxShapeFields> shapeInts{};
            uint8_t shapeFieldCount = 0;
        };

        SpecializedCollectionStorage(Kind kind, PrimitiveTypeTag keyTag, size_t initialCapacity = 32);
        SpecializedCollectionStorage(Kind kind, ShapeDescriptor shape, size_t initialCapacity = 32);

        Kind getKind() const noexcept { return kind_; }
        Mode getMode() const noexcept { return mode_; }
        PrimitiveTypeTag getKeyTag() const noexcept { return keyTag_; }
        const ShapeDescriptor& getShape() const noexcept { return shape_; }
        size_t size() const noexcept { return count_; }
        bool empty() const noexcept { return count_ == 0; }
        void clear();

        bool put(const Value& keyValue, const Value& value, Value& oldValue);
        bool get(const Value& keyValue, Value& out) const;
        bool containsKey(const Value& keyValue) const;
        bool remove(const Value& keyValue);
        bool containsStoredValue(const Value& value) const;

        bool add(const Value& item);
        bool contains(const Value& item) const;

        std::shared_ptr<NativeArray> materializeKeys(environment::Environment* env) const;
        std::shared_ptr<NativeArray> materializeValues() const;

        void visitReferences(const std::function<void(void*)>& callback) const;
        bool isSpecializedMethod(const std::string& methodName, size_t argCount) const;

        static bool isSpecializableKeyTag(PrimitiveTypeTag tag) noexcept;

        // Phase 1 (MYT-273 follow-up): user value-class shape specialization.
        // Eligibility mirrors MYT-274 synthesis gate so storage's Horner-31
        // hash and field-wise compare are semantically equivalent to the
        // synthesized user-visible equals/hashCode.
        static bool isSpecializableShape(const runtimeTypes::klass::ClassDefinition* classDef);
        static ShapeDescriptor buildShapeDescriptor(
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef);

    private:
        struct Entry
        {
            bool occupied = false;
            Key key;
            Value value;
        };

        Kind kind_;
        Mode mode_ = Mode::PRIMITIVE;
        PrimitiveTypeTag keyTag_;
        ShapeDescriptor shape_;
        std::vector<Entry> entries_;
        size_t count_ = 0;

        bool extractKey(const Value& value, Key& out) const;
        bool extractPrimitiveKey(const Value& value, Key& out) const;
        bool extractShapeKey(const Value& value, Key& out) const;
        bool findSlot(const Key& key, size_t& slot, bool& found) const;
        void insertKnownNew(Key key, Value value);
        void resize(size_t newCapacity);
        void eraseSlot(size_t slot);
        int64_t rawHash(const Key& key) const;
        bool keysEqual(const Key& a, const Key& b) const;
        Value boxKey(const Key& key, environment::Environment* env) const;
        Value boxPrimitiveKey(const Key& key, environment::Environment* env) const;
        Value boxShapeKey(const Key& key) const;
    };
}
