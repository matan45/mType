#pragma once
#include "../NativeArray.hpp"
#include <cstddef>
#include "../ValueType.hpp"
#include "object/FlatMultiObjectArray.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../environment/registry/ClassRegistry.hpp"
#include <memory>
#include <vector>
#include <string>

namespace mType
{
    namespace value
    {
        namespace arrays
        {
            /**
             * @brief Factory for creating optimized array instances
             *
             * Central factory for all array creation in mType. Automatically selects
             * optimal storage based on element type, size, and dimensionality:
             *
             * - int/float/bool + size≥16 → SIMD-optimized storage
             * - string + size≥16 → StringPool-optimized storage
             * - object + size≥16 + ClassDefinition → SoA-optimized storage
             * - multi-dimensional objects → FlatMultiObjectArray with SoA
             *
             * Design Principles:
             * - Factory Pattern: Centralized creation logic
             * - Strategy Pattern: Automatic selection of optimal implementation
             * - Transparent Optimization: Users get best performance without code changes
             * - Backward Compatible: Falls back gracefully when optimization not possible
             */
            class ArrayFactory
            {
            public:
                /**
                 * @brief Create 1D array with automatic optimization
                 *
                 * Automatically selects optimal storage mode:
                 * - Primitives (int/float/bool): SIMD if size ≥ 16
                 * - Strings: StringPool if size ≥ 16
                 * - Objects: SoA if size ≥ 16 AND ClassDefinition provided
                 * - Otherwise: Heterogeneous storage
                 *
                 * @param size Array size
                 * @param elemType Element ValueType
                 * @param elemTypeName Element type name (for objects)
                 * @param classDef ClassDefinition for SoA optimization (optional)
                 * @param classRegistry ClassRegistry for automatic ClassDefinition lookup (optional)
                 * @return Shared pointer to optimized NativeArray
                 */
                static std::shared_ptr<::value::NativeArray> create1DArray(
                    size_t size,
                    ::value::ValueType elemType,
                    const std::string& elemTypeName = "",
                    std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef = nullptr,
                    environment::registry::ClassRegistry* classRegistry = nullptr
                );

                /**
                 * @brief Create multi-dimensional array with automatic optimization
                 *
                 * For object arrays with ClassDefinition, creates FlatMultiObjectArray with SoA.
                 * For other types, uses existing FlatMultiArray or SparseMultiArray.
                 *
                 * @param dimensions Array dimensions [n1, n2, n3, ...]
                 * @param elemType Element ValueType
                 * @param elemTypeName Element type name
                 * @param classDef ClassDefinition for object arrays (optional)
                 * @param classRegistry ClassRegistry for automatic lookup (optional)
                 * @return Value containing optimized multi-dimensional array
                 */
                static ::value::Value createMultiDimensionalArray(
                    const std::vector<size_t>& dimensions,
                    ::value::ValueType elemType,
                    const std::string& elemTypeName = "",
                    std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef = nullptr,
                    environment::registry::ClassRegistry* classRegistry = nullptr
                );

                /**
                 * @brief Create array from literal with type detection
                 *
                 * Analyzes literal elements to detect optimal storage:
                 * - Homogeneous primitives → SIMD if size ≥ 16
                 * - Homogeneous strings → StringPool if size ≥ 16
                 * - Homogeneous objects → SoA if size ≥ 16
                 * - Heterogeneous → Standard storage
                 *
                 * @param elements Evaluated literal elements
                 * @param expectedType Expected element type
                 * @param elemTypeName Element type name (for objects)
                 * @param classRegistry ClassRegistry for automatic lookup (optional)
                 * @return Shared pointer to optimized NativeArray
                 */
                static std::shared_ptr<::value::NativeArray> createFromLiteral(
                    const std::vector<::value::Value>& elements,
                    ::value::ValueType expectedType,
                    const std::string& elemTypeName = "",
                    environment::registry::ClassRegistry* classRegistry = nullptr
                );

                /**
                 * @brief Check if optimization is beneficial for given parameters
                 *
                 * Returns true if optimization would provide meaningful benefit:
                 * - Size meets minimum threshold (≥16)
                 * - Type supports optimization (primitives, strings, objects with ClassDef)
                 * - CPU supports required instructions (for SIMD)
                 *
                 * @param size Array size
                 * @param elemType Element type
                 * @param classDef ClassDefinition (for object arrays)
                 * @return true if optimization recommended
                 */
                static bool shouldOptimize(
                    size_t size,
                    ::value::ValueType elemType,
                    std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef = nullptr
                );

                /**
                 * @brief Get storage mode that would be selected for given parameters
                 *
                 * Useful for testing and diagnostics.
                 *
                 * @param size Array size
                 * @param elemType Element type
                 * @param classDef ClassDefinition (for object arrays)
                 * @return Expected storage mode
                 */
                static std::string getExpectedStorageMode(
                    size_t size,
                    ::value::ValueType elemType,
                    std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef = nullptr
                );

            private:
                /**
                 * @brief Attempt to resolve ClassDefinition from type name
                 *
                 * @param typeName Class name
                 * @param classRegistry ClassRegistry to query
                 * @return ClassDefinition if found, nullptr otherwise
                 */
                static std::shared_ptr<runtimeTypes::klass::ClassDefinition> resolveClassDefinition(
                    const std::string& typeName,
                    environment::registry::ClassRegistry* classRegistry
                );

                /**
                 * @brief Check if ClassDefinition has nested object or array fields
                 *
                 * FlatMultiObjectArray (SoA optimization) doesn't support fields that are
                 * themselves OBJECT or ARRAY types. This function checks if a class has
                 * such fields and returns true if it does, indicating we should fall back
                 * to regular FlatMultiArray instead.
                 *
                 * @param classDef ClassDefinition to check
                 * @return true if class has OBJECT or ARRAY fields (cannot use SoA)
                 */
                static bool hasNestedObjectOrArrayFields(
                    std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef
                );

                /**
                 * @brief Check if size meets optimization threshold
                 */
                static bool meetsThreshold(size_t size);

                /**
                 * @brief Calculate total size for multi-dimensional array
                 */
                static size_t calculateTotalSize(const std::vector<size_t>& dimensions);
            };
        } // namespace arrays
    } // namespace value
} // namespace mType
