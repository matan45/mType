#pragma once
#include "../detection/CPUFeatures.hpp"
#include <cstddef>
#include <cstdint>

namespace mType {
namespace value {
namespace simd {

/**
 * Runtime dispatcher for SIMD operations.
 * Selects optimal implementation based on CPU capabilities.
 *
 * Design Principles:
 * - Strategy Pattern: Runtime algorithm selection
 * - Singleton Pattern: Single instance for entire application
 * - Performance: Zero overhead after initialization
 */
class SIMDDispatcher {
public:
    // Get singleton instance
    static SIMDDispatcher& instance();

    // Query current instruction set
    CPUFeatures::InstructionSet getInstructionSet() const {
        return instructionSet_;
    }

    // Integer array operations
    void addInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count) const;
    void subtractInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count) const;
    void multiplyInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count) const;
    void divideInt32(const int32_t* a, const int32_t* b, int32_t* result, size_t count) const;

    // Integer array with scalar
    void addInt32Scalar(const int32_t* a, int32_t scalar, int32_t* result, size_t count) const;
    void multiplyInt32Scalar(const int32_t* a, int32_t scalar, int32_t* result, size_t count) const;

    // Integer reductions
    int32_t sumInt32(const int32_t* data, size_t count) const;
    int32_t minInt32(const int32_t* data, size_t count) const;
    int32_t maxInt32(const int32_t* data, size_t count) const;

    // Float array operations
    void addFloat(const float* a, const float* b, float* result, size_t count) const;
    void subtractFloat(const float* a, const float* b, float* result, size_t count) const;
    void multiplyFloat(const float* a, const float* b, float* result, size_t count) const;
    void divideFloat(const float* a, const float* b, float* result, size_t count) const;

    // Float array with scalar
    void addFloatScalar(const float* a, float scalar, float* result, size_t count) const;
    void multiplyFloatScalar(const float* a, float scalar, float* result, size_t count) const;

    // Float reductions
    float sumFloat(const float* data, size_t count) const;
    float minFloat(const float* data, size_t count) const;
    float maxFloat(const float* data, size_t count) const;

private:
    SIMDDispatcher();
    ~SIMDDispatcher() = default;

    // Disable copy and move
    SIMDDispatcher(const SIMDDispatcher&) = delete;
    SIMDDispatcher& operator=(const SIMDDispatcher&) = delete;

    CPUFeatures::InstructionSet instructionSet_;
};

} // namespace simd
} // namespace value
} // namespace mType
