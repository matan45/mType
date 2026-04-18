#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include "ValueType.hpp"
#include "PrimitiveTypeTag.hpp"

namespace runtimeTypes::klass
{
    class ClassDefinition;
}

namespace value
{
    /**
     * @brief Lightweight value type object with copy semantics
     *
     * ValueObject is a compact alternative to ObjectInstance for value classes.
     * It stores fields in a vector indexed by position (O(1) access) instead of
     * a hash map, and does not register with the GC or maintain a method cache.
     *
     * Memory comparison:
     * - ObjectInstance: ~230+ bytes overhead (hash map, method cache, GC, dual storage)
     * - ValueObject: ~48-80 bytes overhead (vector fields, ClassDefinition pointer)
     *
     * Value semantics:
     * - Assignment creates a deep copy
     * - Structural equality by default
     * - Implicitly final (no inheritance)
     */
    class ValueObject
    {
    private:
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDefinition;
        std::vector<Value> fields; // Indexed by field position (no hash map)
        std::unordered_map<std::string, std::string> genericTypeBindings; // Only for generic value classes
        PrimitiveTypeTag primitiveTag_ = PrimitiveTypeTag::NONE;

    public:
        explicit ValueObject(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef);
        ValueObject(const ValueObject& other);
        ValueObject& operator=(const ValueObject& other);
        ValueObject(ValueObject&& other) noexcept = default;
        ValueObject& operator=(ValueObject&& other) noexcept = default;

        // Field access by index (O(1))
        const Value& getFieldByIndex(size_t index) const;
        void setFieldByIndex(size_t index, const Value& value);

        // Field access by name (uses ClassDefinition field index lookup)
        Value getFieldValue(const std::string& fieldName) const;
        void setField(const std::string& fieldName, const Value& value);

        // Check if field exists
        bool hasField(const std::string& fieldName) const;

        // Type info
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> getClassDefinition() const;
        const std::string& getClassName() const;

        // Generic type bindings
        void setGenericTypeBinding(const std::string& param, const std::string& type);
        const std::unordered_map<std::string, std::string>& getGenericTypeBindings() const;

        // Deep copy for value semantics
        std::shared_ptr<ValueObject> deepCopy() const;

        // Structural equality
        bool equals(const ValueObject& other) const;

        // Primitive type tag for fast type checking (avoids string comparisons)
        PrimitiveTypeTag getPrimitiveTag() const { return primitiveTag_; }

        // Field count
        size_t getFieldCount() const;

        // Access the raw fields vector (for serialization, iteration)
        const std::vector<Value>& getFields() const;

        // MYT-169: byte offset of the classDefinition shared_ptr from the start
        // of a ValueObject, consumed by JIT shape-guard emission to bypass the
        // jit_extract_classdef helper call. Defined out-of-line in the .cpp.
        static size_t classDefinitionMemberOffset() noexcept;
    };
}
