#pragma once

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define MTYPE_PLATFORM_WINDOWS
#elif defined(__linux__)
#define MTYPE_PLATFORM_LINUX
#elif defined(__APPLE__)
#define MTYPE_PLATFORM_MACOS
#endif

// Architecture detection
#if defined(__x86_64__) || defined(_M_X64)
#define MTYPE_ARCH_X64
#elif defined(__aarch64__) || defined(_M_ARM64)
#define MTYPE_ARCH_ARM64
#endif

// SIMD instruction set detection
#ifdef MTYPE_ARCH_X64
// SSE2 is baseline for x64
#define MTYPE_SIMD_SSE2

#if defined(__AVX2__) || (defined(_MSC_VER) && defined(__AVX2__))
#define MTYPE_SIMD_AVX2
#endif

// Include appropriate headers
#ifdef MTYPE_SIMD_SSE2
#include <emmintrin.h>  // SSE2
#include <smmintrin.h>  // SSE4.1
#endif

#ifdef MTYPE_SIMD_AVX2
#include <immintrin.h>  // AVX, AVX2
#endif
#endif

#ifdef MTYPE_ARCH_ARM64
#define MTYPE_SIMD_NEON
#include <arm_neon.h>
#endif

// SIMD configuration
// Only define MTYPE_SIMD_ENABLED if not already defined (e.g., from build system)
#ifndef MTYPE_SIMD_ENABLED
#ifdef MTYPE_SIMD_SSE2
#define MTYPE_SIMD_ENABLED
#elif defined(MTYPE_SIMD_NEON)
#define MTYPE_SIMD_ENABLED
#endif
#endif

// SIMD vector widths (elements per vector)
namespace mType
{
    namespace value
    {
        namespace simd
        {
            struct SIMDWidth
            {
#ifdef MTYPE_SIMD_AVX2
                static constexpr size_t INT32 = 8; // 256-bit / 32-bit
                static constexpr size_t FLOAT = 8; // 256-bit / 32-bit
                static constexpr size_t DOUBLE = 4; // 256-bit / 64-bit
#elif defined(MTYPE_SIMD_SSE2)
                static constexpr size_t INT32 = 4; // 128-bit / 32-bit
                static constexpr size_t FLOAT = 4; // 128-bit / 32-bit
                static constexpr size_t DOUBLE = 2; // 128-bit / 64-bit
#elif defined(MTYPE_SIMD_NEON)
                static constexpr size_t INT32 = 4; // 128-bit / 32-bit
                static constexpr size_t FLOAT = 4; // 128-bit / 32-bit
                static constexpr size_t DOUBLE = 2; // 128-bit / 64-bit
#else
                static constexpr size_t INT32 = 1; // Scalar fallback
                static constexpr size_t FLOAT = 1; // Scalar fallback
                static constexpr size_t DOUBLE = 1; // Scalar fallback
#endif
            };

            // Alignment requirements
            struct SIMDAlignment
            {
#ifdef MTYPE_SIMD_AVX2
                static constexpr size_t BYTES = 32; // 256-bit alignment
#elif defined(MTYPE_SIMD_SSE2) || defined(MTYPE_SIMD_NEON)
                static constexpr size_t BYTES = 16; // 128-bit alignment
#else
                static constexpr size_t BYTES = 8; // Standard alignment
#endif
            };

            // Threshold for SIMD usage (elements)
            struct SIMDThreshold
            {
                static constexpr size_t MIN_ELEMENTS = 16; // Minimum array size for SIMD benefit
            };
        } // namespace simd
    } // namespace value
} // namespace mType
