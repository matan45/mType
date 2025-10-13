#pragma once
#include "../../NativeArray.hpp"
#include "../../simd/dispatch/SIMDDispatcher.hpp"
#include <memory>
#include <stdexcept>

namespace mType {
namespace value {

// Forward declaration of NativeArray
class NativeArray;

namespace arrays {

// Bring NativeArray into scope
using ::value::NativeArray;
using ::value::Value;
using ::value::ValueType;

/**
 * @brief Helper class for array operations with automatic SIMD optimization
 *
 * Provides high-level array operations that automatically leverage SIMD
 * when the underlying NativeArray uses SIMD storage.
 *
 * Design Principles:
 * - Strategy Pattern: Dispatches to SIMD or scalar based on storage mode
 * - Static Methods: Stateless utility functions
 * - Type Safety: Validates type compatibility before operations
 */
class ArrayOperations {
public:
    // Binary operations: Array op Array
    static std::shared_ptr<NativeArray> add(
        const NativeArray& lhs,
        const NativeArray& rhs
    );

    static std::shared_ptr<NativeArray> subtract(
        const NativeArray& lhs,
        const NativeArray& rhs
    );

    static std::shared_ptr<NativeArray> multiply(
        const NativeArray& lhs,
        const NativeArray& rhs
    );

    static std::shared_ptr<NativeArray> divide(
        const NativeArray& lhs,
        const NativeArray& rhs
    );

    // Scalar operations: Array op Scalar
    static std::shared_ptr<NativeArray> addScalar(
        const NativeArray& array,
        const Value& scalar
    );

    static std::shared_ptr<NativeArray> subtractScalar(
        const NativeArray& array,
        const Value& scalar,
        bool scalarOnLeft = false
    );

    static std::shared_ptr<NativeArray> multiplyScalar(
        const NativeArray& array,
        const Value& scalar
    );

    static std::shared_ptr<NativeArray> divideScalar(
        const NativeArray& array,
        const Value& scalar,
        bool scalarOnLeft = false
    );

    // Reduction operations
    static Value sum(const NativeArray& array);
    static Value product(const NativeArray& array);
    static Value min(const NativeArray& array);
    static Value max(const NativeArray& array);
    static Value average(const NativeArray& array);

    // Validation
    static bool areCompatible(const NativeArray& lhs, const NativeArray& rhs);
    static bool canApplyScalar(const NativeArray& array, const Value& scalar);

private:
    // SIMD-optimized operations
    static std::shared_ptr<NativeArray> addSIMDInt(
        const NativeArray& lhs,
        const NativeArray& rhs
    );

    static std::shared_ptr<NativeArray> addSIMDFloat(
        const NativeArray& lhs,
        const NativeArray& rhs
    );

    // Scalar fallback operations
    static std::shared_ptr<NativeArray> addHeterogeneous(
        const NativeArray& lhs,
        const NativeArray& rhs
    );

    // Helper: Extract numeric value from Value variant
    template<typename T>
    static bool tryGetNumeric(const Value& val, T& out);
};

} // namespace arrays
} // namespace value
} // namespace mType
