#include "SSE2Ops.hpp"

#ifdef MTYPE_SIMD_SSE2

#include <emmintrin.h>  // SSE2
#include <smmintrin.h>  // SSE4.1 for some operations
#include <algorithm>
#include <limits>

namespace mType {
namespace value {
namespace simd {
namespace sse2 {

// Integer operations
void SSE2Ops::addInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count) {
    size_t i = 0;
    const size_t simdCount = count & ~3;  // Round down to multiple of 4

    // Process 4 integers at a time with SSE2
    for (; i < simdCount; i += 4) {
        __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + i));
        __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b + i));
        __m128i vr = _mm_add_epi32(va, vb);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(result + i), vr);
    }

    // Handle remaining elements (0-3)
    for (; i < count; ++i) {
        result[i] = a[i] + b[i];
    }
}

void SSE2Ops::subtractInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count) {
    size_t i = 0;
    const size_t simdCount = count & ~3;

    for (; i < simdCount; i += 4) {
        __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + i));
        __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b + i));
        __m128i vr = _mm_sub_epi32(va, vb);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(result + i), vr);
    }

    for (; i < count; ++i) {
        result[i] = a[i] - b[i];
    }
}

void SSE2Ops::multiplyInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count) {
    size_t i = 0;
    const size_t simdCount = count & ~3;

    for (; i < simdCount; i += 4) {
        __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + i));
        __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b + i));

        // SSE2 doesn't have 32-bit multiply, use SSE4.1 if available
        #ifdef __SSE4_1__
        __m128i vr = _mm_mullo_epi32(va, vb);
        #else
        // Fallback: multiply pairwise and reconstruct
        __m128i tmp1 = _mm_mul_epu32(va, vb);
        __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(va, 4), _mm_srli_si128(vb, 4));
        __m128i vr = _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0,0,2,0)),
                                        _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0,0,2,0)));
        #endif

        _mm_storeu_si128(reinterpret_cast<__m128i*>(result + i), vr);
    }

    for (; i < count; ++i) {
        result[i] = a[i] * b[i];
    }
}

void SSE2Ops::addInt32Scalar(const int32_t* a, int32_t scalar, int32_t* result, size_t count) {
    size_t i = 0;
    const size_t simdCount = count & ~3;
    __m128i vScalar = _mm_set1_epi32(scalar);

    for (; i < simdCount; i += 4) {
        __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + i));
        __m128i vr = _mm_add_epi32(va, vScalar);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(result + i), vr);
    }

    for (; i < count; ++i) {
        result[i] = a[i] + scalar;
    }
}

void SSE2Ops::multiplyInt32Scalar(const int32_t* a, int32_t scalar, int32_t* result, size_t count) {
    size_t i = 0;
    const size_t simdCount = count & ~3;
    __m128i vScalar = _mm_set1_epi32(scalar);

    for (; i < simdCount; i += 4) {
        __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + i));

        #ifdef __SSE4_1__
        __m128i vr = _mm_mullo_epi32(va, vScalar);
        #else
        // Fallback for pure SSE2
        __m128i tmp1 = _mm_mul_epu32(va, vScalar);
        __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(va, 4), _mm_srli_si128(vScalar, 4));
        __m128i vr = _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0,0,2,0)),
                                        _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0,0,2,0)));
        #endif

        _mm_storeu_si128(reinterpret_cast<__m128i*>(result + i), vr);
    }

    for (; i < count; ++i) {
        result[i] = a[i] * scalar;
    }
}

int32_t SSE2Ops::sumInt32(const int32_t* data, size_t count) {
    size_t i = 0;
    const size_t simdCount = count & ~3;
    __m128i vSum = _mm_setzero_si128();

    // Accumulate 4 integers at a time
    for (; i < simdCount; i += 4) {
        __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + i));
        vSum = _mm_add_epi32(vSum, v);
    }

    // Horizontal sum of 4 lanes
    alignas(16) int32_t temp[4];
    _mm_store_si128(reinterpret_cast<__m128i*>(temp), vSum);
    int32_t sum = temp[0] + temp[1] + temp[2] + temp[3];

    // Handle remaining elements
    for (; i < count; ++i) {
        sum += data[i];
    }

    return sum;
}

int32_t SSE2Ops::minInt32(const int32_t* data, size_t count) {
    if (count == 0) return std::numeric_limits<int32_t>::max();

    size_t i = 0;
    const size_t simdCount = count & ~3;
    __m128i vMin = _mm_set1_epi32(std::numeric_limits<int32_t>::max());

    for (; i < simdCount; i += 4) {
        __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + i));
        #ifdef __SSE4_1__
        vMin = _mm_min_epi32(vMin, v);
        #else
        // SSE2 fallback: compare and blend manually
        __m128i mask = _mm_cmplt_epi32(v, vMin);
        vMin = _mm_or_si128(_mm_and_si128(mask, v), _mm_andnot_si128(mask, vMin));
        #endif
    }

    alignas(16) int32_t temp[4];
    _mm_store_si128(reinterpret_cast<__m128i*>(temp), vMin);
    int32_t minVal = std::min({temp[0], temp[1], temp[2], temp[3]});

    for (; i < count; ++i) {
        if (data[i] < minVal) minVal = data[i];
    }

    return minVal;
}

int32_t SSE2Ops::maxInt32(const int32_t* data, size_t count) {
    if (count == 0) return std::numeric_limits<int32_t>::min();

    size_t i = 0;
    const size_t simdCount = count & ~3;
    __m128i vMax = _mm_set1_epi32(std::numeric_limits<int32_t>::min());

    for (; i < simdCount; i += 4) {
        __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + i));
        #ifdef __SSE4_1__
        vMax = _mm_max_epi32(vMax, v);
        #else
        __m128i mask = _mm_cmpgt_epi32(v, vMax);
        vMax = _mm_or_si128(_mm_and_si128(mask, v), _mm_andnot_si128(mask, vMax));
        #endif
    }

    alignas(16) int32_t temp[4];
    _mm_store_si128(reinterpret_cast<__m128i*>(temp), vMax);
    int32_t maxVal = std::max({temp[0], temp[1], temp[2], temp[3]});

    for (; i < count; ++i) {
        if (data[i] > maxVal) maxVal = data[i];
    }

    return maxVal;
}

// Float operations
void SSE2Ops::addFloat(const float* a, const float* b, float* result, size_t count) {
    size_t i = 0;
    const size_t simdCount = count & ~3;

    for (; i < simdCount; i += 4) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 vr = _mm_add_ps(va, vb);
        _mm_storeu_ps(result + i, vr);
    }

    for (; i < count; ++i) {
        result[i] = a[i] + b[i];
    }
}

void SSE2Ops::subtractFloat(const float* a, const float* b, float* result, size_t count) {
    size_t i = 0;
    const size_t simdCount = count & ~3;

    for (; i < simdCount; i += 4) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 vr = _mm_sub_ps(va, vb);
        _mm_storeu_ps(result + i, vr);
    }

    for (; i < count; ++i) {
        result[i] = a[i] - b[i];
    }
}

void SSE2Ops::multiplyFloat(const float* a, const float* b, float* result, size_t count) {
    size_t i = 0;
    const size_t simdCount = count & ~3;

    for (; i < simdCount; i += 4) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 vr = _mm_mul_ps(va, vb);
        _mm_storeu_ps(result + i, vr);
    }

    for (; i < count; ++i) {
        result[i] = a[i] * b[i];
    }
}

void SSE2Ops::divideFloat(const float* a, const float* b, float* result, size_t count) {
    size_t i = 0;
    const size_t simdCount = count & ~3;

    for (; i < simdCount; i += 4) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 vr = _mm_div_ps(va, vb);
        _mm_storeu_ps(result + i, vr);
    }

    for (; i < count; ++i) {
        result[i] = a[i] / b[i];
    }
}

void SSE2Ops::addFloatScalar(const float* a, float scalar, float* result, size_t count) {
    size_t i = 0;
    const size_t simdCount = count & ~3;
    __m128 vScalar = _mm_set1_ps(scalar);

    for (; i < simdCount; i += 4) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vr = _mm_add_ps(va, vScalar);
        _mm_storeu_ps(result + i, vr);
    }

    for (; i < count; ++i) {
        result[i] = a[i] + scalar;
    }
}

void SSE2Ops::multiplyFloatScalar(const float* a, float scalar, float* result, size_t count) {
    size_t i = 0;
    const size_t simdCount = count & ~3;
    __m128 vScalar = _mm_set1_ps(scalar);

    for (; i < simdCount; i += 4) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vr = _mm_mul_ps(va, vScalar);
        _mm_storeu_ps(result + i, vr);
    }

    for (; i < count; ++i) {
        result[i] = a[i] * scalar;
    }
}

float SSE2Ops::sumFloat(const float* data, size_t count) {
    size_t i = 0;
    const size_t simdCount = count & ~3;
    __m128 vSum = _mm_setzero_ps();

    for (; i < simdCount; i += 4) {
        __m128 v = _mm_loadu_ps(data + i);
        vSum = _mm_add_ps(vSum, v);
    }

    // Horizontal sum
    alignas(16) float temp[4];
    _mm_store_ps(temp, vSum);
    float sum = temp[0] + temp[1] + temp[2] + temp[3];

    for (; i < count; ++i) {
        sum += data[i];
    }

    return sum;
}

float SSE2Ops::minFloat(const float* data, size_t count) {
    if (count == 0) return std::numeric_limits<float>::max();

    size_t i = 0;
    const size_t simdCount = count & ~3;
    __m128 vMin = _mm_set1_ps(std::numeric_limits<float>::max());

    for (; i < simdCount; i += 4) {
        __m128 v = _mm_loadu_ps(data + i);
        vMin = _mm_min_ps(vMin, v);
    }

    alignas(16) float temp[4];
    _mm_store_ps(temp, vMin);
    float minVal = std::min({temp[0], temp[1], temp[2], temp[3]});

    for (; i < count; ++i) {
        if (data[i] < minVal) minVal = data[i];
    }

    return minVal;
}

float SSE2Ops::maxFloat(const float* data, size_t count) {
    if (count == 0) return std::numeric_limits<float>::lowest();

    size_t i = 0;
    const size_t simdCount = count & ~3;
    __m128 vMax = _mm_set1_ps(std::numeric_limits<float>::lowest());

    for (; i < simdCount; i += 4) {
        __m128 v = _mm_loadu_ps(data + i);
        vMax = _mm_max_ps(vMax, v);
    }

    alignas(16) float temp[4];
    _mm_store_ps(temp, vMax);
    float maxVal = std::max({temp[0], temp[1], temp[2], temp[3]});

    for (; i < count; ++i) {
        if (data[i] > maxVal) maxVal = data[i];
    }

    return maxVal;
}

} // namespace sse2
} // namespace simd
} // namespace value
} // namespace mType

#endif // MTYPE_SIMD_SSE2
