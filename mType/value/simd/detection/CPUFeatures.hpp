#pragma once
#include <cstdint>
#include "../SIMDConfig.hpp"

namespace mType {
namespace value {
namespace simd {

/**
 * Runtime CPU feature detection.
 * Detects available SIMD instruction sets at runtime.
 *
 * Design Principles:
 * - Singleton Pattern: Single instance for entire application
 * - Lazy Initialization: Detect features on first access
 * - Thread-Safe: Detection happens once, reads are thread-safe
 */
class CPUFeatures {
public:
    // Get singleton instance
    static const CPUFeatures& instance();

    // x86 features
    bool hasSSE2() const { return sse2_; }
    bool hasSSE4_1() const { return sse4_1_; }
    bool hasAVX() const { return avx_; }
    bool hasAVX2() const { return avx2_; }
    bool hasFMA() const { return fma_; }

    // ARM features
    bool hasNEON() const { return neon_; }

    // Query best available instruction set
    enum class InstructionSet {
        SCALAR,
        SSE2,
        AVX2,
        NEON
    };

    InstructionSet getBestInstructionSet() const;

private:
    CPUFeatures();
    ~CPUFeatures() = default;

    // Disable copy and move
    CPUFeatures(const CPUFeatures&) = delete;
    CPUFeatures& operator=(const CPUFeatures&) = delete;

    void detectFeatures();

#ifdef MTYPE_ARCH_X64
    void cpuid(uint32_t level, uint32_t& eax, uint32_t& ebx,
               uint32_t& ecx, uint32_t& edx);
    bool isOSAVXSupported();
#endif

    bool sse2_ = false;
    bool sse4_1_ = false;
    bool avx_ = false;
    bool avx2_ = false;
    bool fma_ = false;
    bool neon_ = false;
};

} // namespace simd
} // namespace value
} // namespace mType
