#pragma once
#include "../base/IArray.hpp"
#include <cstddef>
#include "../primitive/PrimitiveArray.hpp"
#include "../string/StringArray.hpp"
#include "../../../environment/registry/ClassDefinition.hpp"
#include "../../../value/ObjectInstance.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

namespace mType {
namespace value {
namespace arrays {

/**
 * @brief Base class for Structure-of-Arrays (SoA) object storage
 *
 * Provides common functionality for ObjectArray and FlatMultiObjectArray:
 * - Field-oriented storage management
 * - Object materialization/decomposition
 * - Field validation and access
 * - Memory statistics
 *
 * Design Principles:
 * - Template Method Pattern: Derived classes implement size-specific behavior
 * - DRY Principle: Eliminates 200+ lines of duplicated code
 * - Flyweight Pattern: Shared ClassDefinition across all instances
 * - Column-Store Pattern: Field-oriented storage like databases
 */
class ObjectArrayBase {
public:
    /**
     * @brief Memory statistics for SoA vs AoS comparison
     */
    struct MemoryStats {
        size_t soaMemoryUsage;      // SoA total memory
        size_t aosMemoryUsage;      // AoS equivalent memory (estimated)
        size_t memorySaved;         // Bytes saved
        double savingsPercentage;   // Percentage saved
        size_t objectCount;         // Number of objects
        size_t fieldCount;          // Number of fields
    };

    virtual ~ObjectArrayBase() = default;

    // Field operations (shared implementation)

    /**
     * @brief Check if a field exists in the schema
     */
    bool hasField(const std::string& fieldName) const;

    /**
     * @brief Get all field names in schema
     */
    std::vector<std::string> getFieldNames() const;

    /**
     * @brief Get field array for direct bulk operations
     * @param fieldName Field to get array for
     * @return Shared pointer to field's array, or nullptr if field doesn't exist
     */
    std::shared_ptr<IArray> getFieldArray(const std::string& fieldName) const;

    /**
     * @brief Get the class definition shared by all instances
     */
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> getClassDefinition() const {
        return classDefinition_;
    }

    /**
     * @brief Check if SIMD is supported for any field
     */
    bool supportsSIMD() const;

protected:
    /**
     * @brief Protected constructor (base class)
     * @param classDef Shared class definition for all instances
     */
    explicit ObjectArrayBase(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef
    );

    // Shared member variables
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDefinition_;
    std::unordered_map<std::string, std::shared_ptr<IArray>> fieldArrays_;

    // Shared helper methods

    /**
     * @brief Initialize field arrays based on class definition
     * @param capacity Initial capacity for field arrays
     */
    void initializeFieldArrays(size_t capacity);

    /**
     * @brief Create appropriate array type for field
     * @param field Field definition
     * @param capacity Initial capacity
     * @return Shared pointer to created array
     */
    std::shared_ptr<IArray> createFieldArray(
        const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field,
        size_t capacity
    );

    /**
     * @brief Materialize ObjectInstance from SoA data at index
     * @param linearIndex Index in flattened storage
     * @return Materialized ObjectInstance
     */
    std::shared_ptr<runtimeTypes::klass::ObjectInstance> materializeInstance(
        size_t linearIndex
    ) const;

    /**
     * @brief Decompose ObjectInstance into SoA storage at index
     * @param linearIndex Index in flattened storage
     * @param instance ObjectInstance to decompose
     */
    void decomposeInstance(
        size_t linearIndex,
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& instance
    );

    /**
     * @brief Validate field type matches expected type
     * @param fieldName Field name to validate
     * @param value Value to check
     * @return true if types match, false otherwise
     */
    bool validateFieldType(
        const std::string& fieldName,
        const ::value::Value& value
    ) const;

    /**
     * @brief Get field arrays storage (for derived class access)
     */
    std::unordered_map<std::string, std::shared_ptr<IArray>>& getFieldArraysStorage() {
        return fieldArrays_;
    }

    const std::unordered_map<std::string, std::shared_ptr<IArray>>& getFieldArraysStorage() const {
        return fieldArrays_;
    }
};

} // namespace arrays
} // namespace value
} // namespace mType
