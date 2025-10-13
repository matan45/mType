#pragma once
#include "../../SIMDConfig.hpp"

#ifdef MTYPE_SIMD_SSE2

#include <cstddef>
#include <cstdint>

namespace mType {
namespace value {
namespace simd {
namespace sse2 {

/**
 * SSE2 implementations for array operations.
 * Processes 4 int32/float elements at a time (128-bit vectors).
 *
 * Design Principles:
 * - Platform-Specific: Only compiled when SSE2 available
 * - Performance-Critical: Inline-heavy for zero overhead
 * - Remainder Handling: Falls back to scalar for non-multiple-of-4 sizes
 */
class SSE2Ops {
public:
    // Integer operations
    static void addInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count);
    static void subtractInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count);
    static void multiplyInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count);

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
};

} // namespace sse2
} // namespace simd
} // namespace value
} // namespace mType

#endif // MTYPE_SIMD_SSE2
