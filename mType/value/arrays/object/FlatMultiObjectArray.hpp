#pragma once
#include "../base/IArray.hpp"
#include "../primitive/PrimitiveArray.hpp"
#include "../string/StringArray.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <numeric>
#include <stdexcept>

namespace mType {
namespace value {
namespace arrays {

/**
 * @brief Multi-dimensional Structure-of-Arrays (SoA) storage for object arrays
 *
 * Combines FlatMultiArray's stride-based indexing with ObjectArray's SoA storage.
 * Stores all objects in field-oriented arrays regardless of dimensionality.
 *
 * Memory Layout Example:
 * Person[10][20][30] with fields {id: int, name: string, age: int}
 *
 * Traditional nested arrays: 10 × 20 × 30 = 6000 objects, each with hash map
 * Memory: 6000 × 352 bytes = ~2.1 MB
 *
 * FlatMultiObjectArray:
 * - dimensions: [10, 20, 30]
 * - strides: [600, 30, 1]
 * - id_array: IntArray(6000)      →  24 KB
 * - name_array: StringArray(6000) →  48 KB (pool IDs)
 * - age_array: IntArray(6000)     →  24 KB
 * Total: ~100 KB (95% memory reduction!)
 *
 * Performance Benefits:
 * - SIMD operations across entire field columns
 * - Stride arithmetic for O(1) indexing
 * - Contiguous field storage for cache locality
 * - Single allocation per field (no fragmentation)
 *
 * Design Principles:
 * - Flyweight Pattern: Shared ClassDefinition
 * - Column-Store Pattern: Field-oriented storage
 * - Strategy Pattern: Different array types per field
 * - Stride Indexing: From FlatMultiArray design
 *
 * Supports view semantics: sub-arrays are views into parent data, not copies.
 * Modifications to sub-arrays affect the parent array.
 */
class FlatMultiObjectArray : public std::enable_shared_from_this<FlatMultiObjectArray> {
public:
    /**
     * @brief Construct multi-dimensional object array
     * @param classDef Shared class definition for all instances
     * @param dims Dimensions for each axis [n1, n2, n3, ...]
     */
    explicit FlatMultiObjectArray(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
        const std::vector<size_t>& dims
    );

    // Element access

    /**
     * @brief Get element at multi-dimensional index
     * @param indices Vector of indices [i1, i2, i3, ...]
     * @return Materialized ObjectInstance
     */
    ::value::Value get(const std::vector<size_t>& indices) const;

    /**
     * @brief Set element at multi-dimensional index
     * @param indices Vector of indices [i1, i2, i3, ...]
     * @param value ObjectInstance to decompose and store
     */
    void set(const std::vector<size_t>& indices, const ::value::Value& value);

    /**
     * @brief Get element at linear index (flattened access)
     * @param linearIndex Index in flattened array
     * @return Materialized ObjectInstance
     */
    ::value::Value getLinear(size_t linearIndex) const;

    /**
     * @brief Set element at linear index (flattened access)
     * @param linearIndex Index in flattened array
     * @param value ObjectInstance to store
     */
    void setLinear(size_t linearIndex, const ::value::Value& value);

    // Field-level access (high performance)

    /**
     * @brief Get field value directly at multi-dimensional index
     * @param indices Vector of indices [i1, i2, i3, ...]
     * @param fieldName Field to retrieve
     * @return Field value without materializing ObjectInstance
     */
    ::value::Value getField(const std::vector<size_t>& indices, const std::string& fieldName) const;

    /**
     * @brief Set field value directly at multi-dimensional index
     * @param indices Vector of indices [i1, i2, i3, ...]
     * @param fieldName Field to update
     * @param value New field value
     */
    void setField(const std::vector<size_t>& indices, const std::string& fieldName, const ::value::Value& value);

    /**
     * @brief Get entire field array for batch operations
     * Allows SIMD operations on entire field columns
     * @param fieldName Field to get array for
     * @return Shared pointer to field array
     */
    std::shared_ptr<IArray> getFieldArray(const std::string& fieldName) const;

    // Dimensional information

    /**
     * @brief Get dimensions of the array
     */
    const std::vector<size_t>& getDimensions() const { return dimensions_; }

    /**
     * @brief Get number of dimensions (rank)
     */
    size_t getRank() const { return dimensions_.size(); }

    /**
     * @brief Get total number of objects (flattened)
     */
    size_t totalSize() const { return totalSize_; }

    /**
     * @brief Get size of first dimension
     */
    size_t size() const { return dimensions_.empty() ? 0 : dimensions_[0]; }

    /**
     * @brief Check if array is empty
     */
    bool empty() const { return totalSize_ == 0; }

    // Sub-array slicing

    /**
     * @brief Get sub-array at specified first dimension index
     * Creates a new FlatMultiObjectArray view with reduced dimensions
     * @param index Index for the first dimension
     * @return Shared pointer to sub-array
     */
    std::shared_ptr<FlatMultiObjectArray> getSubArray(size_t index) const;

    // Class and field metadata

    /**
     * @brief Get class definition
     */
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> getClassDefinition() const {
        return classDefinition_;
    }

    /**
     * @brief Get all field names
     */
    std::vector<std::string> getFieldNames() const;

    /**
     * @brief Check if field exists
     */
    bool hasField(const std::string& fieldName) const;

    // Memory statistics

    struct MemoryStats {
        size_t soaMemoryUsage;      // SoA total memory
        size_t aosMemoryUsage;      // AoS equivalent memory (estimated)
        size_t memorySaved;         // Bytes saved
        double savingsPercentage;   // Percentage saved
        size_t objectCount;         // Total objects
        size_t fieldCount;          // Number of fields
        size_t dimensionCount;      // Number of dimensions
    };

    /**
     * @brief Get detailed memory statistics
     */
    MemoryStats getMemoryStats() const;

    /**
     * @brief Check if SIMD is supported for any field
     */
    bool supportsSIMD() const;

private:
    // Shared class definition (Flyweight pattern)
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDefinition_;

    // Multi-dimensional indexing
    std::vector<size_t> dimensions_;  // Size of each dimension [n1, n2, n3, ...]
    std::vector<size_t> strides_;     // Stride for each dimension [s1, s2, s3, ...]
    size_t totalSize_;                // Total number of objects (n1 × n2 × n3 × ...)

    // Field-oriented storage (SoA) - only used if not a view
    std::unordered_map<std::string, std::shared_ptr<IArray>> fieldArrays_;

    // View support: allows sub-arrays to reference parent's data instead of copying
    std::shared_ptr<FlatMultiObjectArray> parent_;  // Parent array (nullptr if this is not a view)
    size_t viewOffset_;                             // Offset into parent's data (0 if not a view)

    /**
     * @brief Check if this is a view into another array
     */
    bool isView() const {
        return parent_ != nullptr;
    }

    /**
     * @brief Get reference to the actual field arrays storage (own or parent's)
     */
    std::unordered_map<std::string, std::shared_ptr<IArray>>& getFieldArraysStorage() {
        return isView() ? parent_->getFieldArraysStorage() : fieldArrays_;
    }

    const std::unordered_map<std::string, std::shared_ptr<IArray>>& getFieldArraysStorage() const {
        return isView() ? parent_->getFieldArraysStorage() : fieldArrays_;
    }

    /**
     * @brief Get the effective offset for data access
     */
    size_t getEffectiveOffset() const {
        return isView() ? viewOffset_ : 0;
    }

    /**
     * @brief Private constructor for creating views (used by getSubArray)
     * @param parentArray Parent array to create view into
     * @param offset Offset into parent's data
     * @param dims Dimensions for this view
     */
    FlatMultiObjectArray(
        std::shared_ptr<FlatMultiObjectArray> parentArray,
        size_t offset,
        const std::vector<size_t>& dims
    );

    /**
     * @brief Calculate linear index from multi-dimensional indices
     * @param indices Vector of indices [i1, i2, i3, ...]
     * @return Linear index into flat arrays
     */
    size_t calculateLinearIndex(const std::vector<size_t>& indices) const;

    /**
     * @brief Calculate strides for each dimension
     * For array[n1][n2][n3], strides are [n2*n3, n3, 1]
     */
    void calculateStrides();

    /**
     * @brief Calculate total number of elements
     */
    size_t calculateTotalSize() const;

    /**
     * @brief Initialize field arrays based on class definition
     */
    void initializeFieldArrays();

    /**
     * @brief Create appropriate array type for field
     */
    std::shared_ptr<IArray> createFieldArray(
        const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field,
        size_t capacity
    );

    /**
     * @brief Materialize ObjectInstance from SoA data at linear index
     */
    std::shared_ptr<runtimeTypes::klass::ObjectInstance> materializeInstance(size_t linearIndex) const;

    /**
     * @brief Decompose ObjectInstance into SoA storage at linear index
     */
    void decomposeInstance(size_t linearIndex, const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& instance);

    /**
     * @brief Validate field type
     */
    bool validateFieldType(const std::string& fieldName, const ::value::Value& value) const;
};

} // namespace arrays
} // namespace value
} // namespace mType
