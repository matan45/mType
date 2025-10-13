#pragma once
#include <cstddef>
#include <cstdint>

namespace mType {
namespace value {
namespace simd {
namespace scalar {

/**
 * Scalar fallback implementations for SIMD operations.
 * Used when SIMD is unavailable or for remainder elements.
 *
 * Design Principles:
 * - Single Responsibility: Pure scalar implementations
 * - No Dependencies: Works on any platform
 * - Reference Implementation: Used for correctness validation
 */
class ScalarOps {
public:
    // Integer operations
    static void addInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count);
    static void subtractInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count);
    static void multiplyInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count);
    static void divideInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count);

    // Integer with scalar
    static void addInt32Scalar(const int32_t* a, int32_t scalar, int32_t* result, size_t count);
    static void multiplyInt32Scalar(const int32_t* a, int32_t scalar, int32_t* result, size_t count);

    // Integer reductions
    static int32_t sumInt32(const int32_t* data, size_t count);
    static int32_t minInt32(const int32_t* data, size_t count);
    static int32_t maxInt32(const int32_t* data, size_t count);

    // Float operations
    static void addFloat(const float* a, const float* b, float* result, size_t count);
    static void subtractFloat(const float* a, const float* b, float* result, size_t count);
    static void multiplyFloat(const float* a, const float* b, float* result, size_t count);
    static void divideFloat(const float* a, const float* b, float* result, size_t count);

    // Float with scalar
    static void addFloatScalar(const float* a, float scalar, float* result, size_t count);
    static void multiplyFloatScalar(const float* a, float scalar, float* result, size_t count);

    // Float reductions
    static float sumFloat(const float* data, size_t count);
    static float minFloat(const float* data, size_t count);
    static float maxFloat(const float* data, size_t count);

    // Boolean operations (stored as int8_t)
    static void andBool(const bool* a, const bool* b, bool* result, size_t count);
    static void orBool(const bool* a, const bool* b, bool* result, size_t count);
    static void notBool(const bool* a, bool* result, size_t count);
    static bool anyTrue(const bool* data, size_t count);
    static bool allTrue(const bool* data, size_t count);
};

} // namespace scalar
} // namespace simd
} // namespace value
} // namespace mType
