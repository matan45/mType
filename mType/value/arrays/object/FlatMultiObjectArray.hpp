#pragma once
#include "ObjectArrayBase.hpp"
#include "../../FlatMultiArray.hpp"
#include <cstddef>
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
 * VIEW SEMANTICS - MEMORY SAFETY CRITICAL:
 * ==========================================
 *
 * Sub-arrays are VIEWS (aliases) into parent data, NOT independent copies!
 *
 * Example demonstrating view behavior:
 * ```cpp
 * auto arr = std::make_shared<FlatMultiObjectArray>(classDef, std::vector<size_t>{10, 20});
 * arr->setField({0, 0}, "id", 42);
 *
 * auto subArr = arr->getSubArray(0);    // Creates VIEW, not copy
 * subArr->setField({0}, "id", 99);      // Modifies arr[0][0].id!
 *
 * arr->getField({0, 0}, "id");  // Returns 99 (changed by view!)
 * ```
 *
 * LIFETIME & OWNERSHIP:
 * - Root array owns fieldArrays_ (SoA storage)
 * - Views hold shared_ptr to root, keeping it alive
 * - Destroying root's external references is SAFE - views keep data alive
 * - All sibling views see each other's modifications immediately
 *
 * THREAD SAFETY:
 * - Concurrent reads: SAFE (if no writes)
 * - Concurrent writes or read+write: UNDEFINED BEHAVIOR
 * - Requires external synchronization for concurrent modification
 */
class FlatMultiObjectArray : public ObjectArrayBase, public std::enable_shared_from_this<FlatMultiObjectArray> {
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

    // Field operations (inherited from ObjectArrayBase):
    // - getFieldArray()
    // - getFieldNames()
    // - hasField()
    // - getClassDefinition()

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
     *
     * Note: This is intentionally non-const because it creates a modifiable view that can
     * alter the parent array's data, making it logically a non-const operation.
     *
     * @param index Index for the first dimension
     * @return Shared pointer to sub-array
     */
    std::shared_ptr<FlatMultiObjectArray> getSubArray(size_t index);

    // Memory statistics

    /**
     * @brief Memory statistics extended with dimension count
     */
    struct MultiDimMemoryStats : public ObjectArrayBase::MemoryStats {
        size_t dimensionCount;      // Number of dimensions
    };

    /**
     * @brief Get detailed memory statistics
     */
    MultiDimMemoryStats getMemoryStats() const;

private:
    // Field-oriented storage inherited from ObjectArrayBase:
    // - classDefinition_
    // - fieldArrays_

    // Multi-dimensional indexing
    std::vector<size_t> dimensions_;  // Size of each dimension [n1, n2, n3, ...]
    std::vector<size_t> strides_;     // Stride for each dimension [s1, s2, s3, ...]
    size_t totalSize_;                // Total number of objects (n1 × n2 × n3 × ...)

    // View support: allows sub-arrays to reference parent's data instead of copying
    std::shared_ptr<FlatMultiObjectArray> parent_;  // Parent array (nullptr if this is not a view)
    size_t viewOffset_;                             // Offset into parent's data (0 if not a view)

    // MYT-378: heterogeneous fallback. Lives ONLY on the root (parent_ == nullptr).
    // When a value whose exact class != classDefinition_ is stored, the root's SoA
    // columns are materialized into hetero_ and all subsequent access routes there,
    // preserving subtype fields and polymorphic identity. Views always delegate to
    // the root via the accessors below, so a fallback triggered through any view is
    // visible to all sibling views immediately (same invariant as fieldArrays_).
    bool heterogeneous_ = false;                       // root-authoritative mode flag
    std::unique_ptr<::value::FlatMultiArray> hetero_;  // Value store, root only

    /**
     * @brief Check if this is a view into another array
     */
    bool isView() const {
        return parent_ != nullptr;
    }

    /**
     * @brief Whether the (root) array has fallen back to heterogeneous storage
     */
    bool isHeterogeneous() const {
        return isView() ? parent_->isHeterogeneous() : heterogeneous_;
    }

    /**
     * @brief Get the root's heterogeneous Value store (nullptr if not converted)
     */
    ::value::FlatMultiArray* heteroStore() {
        return isView() ? parent_->heteroStore() : hetero_.get();
    }

    const ::value::FlatMultiArray* heteroStore() const {
        return isView() ? parent_->heteroStore() : hetero_.get();
    }

    /**
     * @brief MYT-378: migrate the ROOT's SoA columns into hetero_ Value storage.
     * Delegates to the root first, then materializes every existing element so
     * already-stored (exact-class) objects keep their identity. Idempotent.
     */
    void convertToHeterogeneous();

    /**
     * @brief MYT-378: store @p value at an already-resolved effective (flat,
     * offset-applied) index, applying the SoA-vs-heterogeneous decision. Shared
     * by set() and setLinear() so the fallback rule lives in exactly one place.
     */
    void storeAt(size_t effectiveIndex, const ::value::Value& value);

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

    // Helper methods inherited from ObjectArrayBase:
    // - initializeFieldArrays(size_t capacity)
    // - createFieldArray()
    // - materializeInstance()
    // - decomposeInstance()
    // - validateFieldType()
};

} // namespace arrays
} // namespace value
} // namespace mType
