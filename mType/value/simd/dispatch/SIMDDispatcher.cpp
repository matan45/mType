#include "SIMDDispatcher.hpp"
#include "../primitives/scalar/ScalarOps.hpp"

#ifdef MTYPE_SIMD_SSE2
#include "../primitives/sse2/SSE2Ops.hpp"
#endif

namespace mType {
namespace value {
namespace simd {

SIMDDispatcher::SIMDDispatcher() {
    const CPUFeatures& features = CPUFeatures::instance();
    instructionSet_ = features.getBestInstructionSet();
}

SIMDDispatcher& SIMDDispatcher::instance() {
    static SIMDDispatcher instance;
    return instance;
}

// Integer operations
void SIMDDispatcher::addInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        sse2::SSE2Ops::addInt32(a, b, result, count);
        return;
    }
#endif
    scalar::ScalarOps::addInt32(a, b, result, count);
}

void SIMDDispatcher::subtractInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        sse2::SSE2Ops::subtractInt32(a, b, result, count);
        return;
    }
#endif
    scalar::ScalarOps::subtractInt32(a, b, result, count);
}

void SIMDDispatcher::multiplyInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        sse2::SSE2Ops::multiplyInt32(a, b, result, count);
        return;
    }
#endif
    scalar::ScalarOps::multiplyInt32(a, b, result, count);
}

void SIMDDispatcher::divideInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count) const {
    // Division doesn't have SIMD benefit, always use scalar
    scalar::ScalarOps::divideInt32(a, b, result, count);
}

void SIMDDispatcher::addInt32Scalar(const int32_t* a, int32_t scalar, int32_t* result, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        sse2::SSE2Ops::addInt32Scalar(a, scalar, result, count);
        return;
    }
#endif
    scalar::ScalarOps::addInt32Scalar(a, scalar, result, count);
}

void SIMDDispatcher::multiplyInt32Scalar(const int32_t* a, int32_t scalar, int32_t* result, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        sse2::SSE2Ops::multiplyInt32Scalar(a, scalar, result, count);
        return;
    }
#endif
    scalar::ScalarOps::multiplyInt32Scalar(a, scalar, result, count);
}

int32_t SIMDDispatcher::sumInt32(const int32_t* data, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        return sse2::SSE2Ops::sumInt32(data, count);
    }
#endif
    return scalar::ScalarOps::sumInt32(data, count);
}

int32_t SIMDDispatcher::minInt32(const int32_t* data, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        return sse2::SSE2Ops::minInt32(data, count);
    }
#endif
    return scalar::ScalarOps::minInt32(data, count);
}

int32_t SIMDDispatcher::maxInt32(const int32_t* data, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        return sse2::SSE2Ops::maxInt32(data, count);
    }
#endif
    return scalar::ScalarOps::maxInt32(data, count);
}

// Float operations
void SIMDDispatcher::addFloat(const float* a, const float* b, float* result, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        sse2::SSE2Ops::addFloat(a, b, result, count);
        return;
    }
#endif
    scalar::ScalarOps::addFloat(a, b, result, count);
}

void SIMDDispatcher::subtractFloat(const float* a, const float* b, float* result, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        sse2::SSE2Ops::subtractFloat(a, b, result, count);
        return;
    }
#endif
    scalar::ScalarOps::subtractFloat(a, b, result, count);
}

void SIMDDispatcher::multiplyFloat(const float* a, const float* b, float* result, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        sse2::SSE2Ops::multiplyFloat(a, b, result, count);
        return;
    }
#endif
    scalar::ScalarOps::multiplyFloat(a, b, result, count);
}

void SIMDDispatcher::divideFloat(const float* a, const float* b, float* result, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        sse2::SSE2Ops::divideFloat(a, b, result, count);
        return;
    }
#endif
    scalar::ScalarOps::divideFloat(a, b, result, count);
}

void SIMDDispatcher::addFloatScalar(const float* a, float scalar, float* result, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        sse2::SSE2Ops::addFloatScalar(a, scalar, result, count);
        return;
    }
#endif
    scalar::ScalarOps::addFloatScalar(a, scalar, result, count);
}

void SIMDDispatcher::multiplyFloatScalar(const float* a, float scalar, float* result, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        sse2::SSE2Ops::multiplyFloatScalar(a, scalar, result, count);
        return;
    }
#endif
    scalar::ScalarOps::multiplyFloatScalar(a, scalar, result, count);
}

float SIMDDispatcher::sumFloat(const float* data, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        return sse2::SSE2Ops::sumFloat(data, count);
    }
#endif
    return scalar::ScalarOps::sumFloat(data, count);
}

float SIMDDispatcher::minFloat(const float* data, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        return sse2::SSE2Ops::minFloat(data, count);
    }
#endif
    return scalar::ScalarOps::minFloat(data, count);
}

float SIMDDispatcher::maxFloat(const float* data, size_t count) const {
#ifdef MTYPE_SIMD_SSE2
    if (instructionSet_ >= CPUFeatures::InstructionSet::SSE2) {
        return sse2::SSE2Ops::maxFloat(data, count);
    }
#endif
    return scalar::ScalarOps::maxFloat(data, count);
}

} // namespace simd
} // namespace value
} // namespace mType
