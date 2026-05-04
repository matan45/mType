#pragma once
#include "../base/IArray.hpp"
#include <cstddef>
#include "ObjectArrayBase.hpp"
#include <memory>
#include <string>
#include <stdexcept>

namespace mType {
namespace value {
namespace arrays {

/**
 * @brief Structure-of-Arrays (SoA) storage for homogeneous object arrays
 *
 * Memory Optimization:
 * - Field-oriented storage: One array per field instead of array-of-objects
 * - Eliminates per-object hash map overhead (24-64 bytes saved per object)
 * - Better cache locality when accessing same field across objects
 * - 40-60% memory reduction for object arrays
 *
 * Performance Benefits:
 * - SIMD operations on primitive fields (int, float, bool)
 * - StringPool optimization for string fields
 * - Vectorized field access and updates
 * - Batch operations on entire columns
 *
 * Design Principles:
 * - Flyweight Pattern: Share ClassDefinition across all instances
 * - Column-Store Pattern: Field-oriented storage like databases
 * - Strategy Pattern: Different array types per field type
 * - Lazy Materialization: Reconstruct ObjectInstance only when needed
 *
 * Example:
 * // AoS (current): [{id:1, name:"A"}, {id:2, name:"B"}]
 * // SoA (this):    {id: [1,2], name: ["A","B"]}
 */
class ObjectArray : public IArray, public ObjectArrayBase {
public:
    /**
     * @brief Construct ObjectArray for a specific class type
     * @param classDef Shared class definition for all instances
     * @param initialSize Number of object instances to allocate
     */
    explicit ObjectArray(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
        size_t initialSize = 0
    );

    // IArray implementation
    size_t size() const override;
    size_t capacity() const override;
    bool empty() const override;

    std::string elementTypeName() const override;
    ::value::ValueType elementType() const override;

    ::value::Value get(size_t index) const override;
    void set(size_t index, const ::value::Value& value) override;

    void reserve(size_t newCapacity) override;
    void resize(size_t newSize) override;
    void clear() override;

    bool supportsSIMD() const override { return ObjectArrayBase::supportsSIMD(); }
    size_t simdWidth() const override;

    std::unique_ptr<IArray> clone() const override;

    // ObjectArray-specific operations

    /**
     * @brief Get field value directly without materializing ObjectInstance
     * @param index Object index in array
     * @param fieldName Field name to retrieve
     * @return Field value at index
     */
    ::value::Value getField(size_t index, const std::string& fieldName) const;

    /**
     * @brief Set field value directly without materializing ObjectInstance
     * @param index Object index in array
     * @param fieldName Field name to update
     * @param value New field value
     */
    void setField(size_t index, const std::string& fieldName, const ::value::Value& value);

    // Field metadata (inherited from ObjectArrayBase):
    // - hasField()
    // - getFieldNames()
    // - getFieldArray()
    // - getClassDefinition()

    /**
     * @brief Calculate memory usage for this ObjectArray
     * @return Total bytes used (field arrays + overhead)
     */
    size_t getMemoryUsage() const;

    /**
     * @brief Calculate memory savings vs Array-of-Structures
     * @return Estimated bytes saved
     */
    size_t getMemorySavings() const;

    /**
     * @brief Get detailed memory statistics (MemoryStats struct defined in ObjectArrayBase)
     */
    ObjectArrayBase::MemoryStats getMemoryStats() const;

    // PERFORMANCE OPTIMIZATION: Unchecked access methods

    /**
     * @brief Unchecked field access (internal use)
     * Bounds check must be done by caller
     * Performance: ~8-10 ns (vs ~200 ns for get() which materializes)
     */
    inline ::value::Value getFieldUnchecked(size_t index, const std::string& fieldName) const noexcept
    {
        auto it = fieldArrays_.find(fieldName);
        if (it != fieldArrays_.end())
        {
            return it->second->get(index);
        }
        return std::monostate{};
    }

    /**
     * @brief Unchecked field set (internal use)
     * Bounds check and type check must be done by caller
     * Performance: ~8-10 ns (vs ~200 ns for set())
     */
    inline void setFieldUnchecked(size_t index, const std::string& fieldName, const ::value::Value& value) noexcept
    {
        auto it = fieldArrays_.find(fieldName);
        if (it != fieldArrays_.end())
        {
            it->second->set(index, value);
        }
    }

    /**
     * @brief Unchecked get with object materialization (internal use)
     * Bounds check must be done by caller
     * WARNING: Still expensive due to materialization!
     * Performance: ~180-200 ns (vs ~200-220 ns for get())
     * Note: Can throw from materializeInstance() (std::bad_alloc, etc.)
     */
    inline ::value::Value getUnchecked(size_t index) const
    {
        return materializeInstance(index);
    }

private:
    // Field-oriented storage inherited from ObjectArrayBase:
    // - classDefinition_
    // - fieldArrays_
    // - initializeFieldArrays()
    // - createFieldArray()
    // - materializeInstance()
    // - decomposeInstance()
    // - validateFieldType()

    // ObjectArray-specific members
    size_t size_;      // Number of object instances
    size_t capacity_;  // Capacity for growth
};

} // namespace arrays
} // namespace value
} // namespace mType
