#pragma once

#include <cstdint>
#include <climits>
#include <string>
#include "../../../errors/RuntimeException.hpp"

namespace vm::runtime::utils
{
    /**
     * Overflow-checked int64 arithmetic helpers.
     *
     * Signed integer overflow is undefined behavior in C++, so we cannot
     * detect it after the fact by inspecting the result. On GCC and Clang
     * we use the well-defined `__builtin_*_overflow` intrinsics. On MSVC
     * (which has no equivalent intrinsic for signed 64-bit) we test the
     * operation against the int64 range *before* performing it, so the
     * actual arithmetic never invokes UB.
     *
     * On overflow we throw RuntimeException so the VM can surface a clean
     * script error instead of crashing or wrapping silently.
     */

#if defined(__GNUC__) || defined(__clang__)
    #define MTYPE_HAS_BUILTIN_OVERFLOW 1
#else
    #define MTYPE_HAS_BUILTIN_OVERFLOW 0
#endif

    inline int64_t checkedAdd64(int64_t a, int64_t b) {
#if MTYPE_HAS_BUILTIN_OVERFLOW
        int64_t result;
        if (__builtin_add_overflow(a, b, &result)) {
            throw errors::RuntimeException("Integer overflow in addition");
        }
        return result;
#else
        // Pre-check: would a + b leave the int64 range?
        if ((b > 0 && a > INT64_MAX - b) ||
            (b < 0 && a < INT64_MIN - b)) {
            throw errors::RuntimeException("Integer overflow in addition");
        }
        return a + b;
#endif
    }

    inline int64_t checkedSub64(int64_t a, int64_t b) {
#if MTYPE_HAS_BUILTIN_OVERFLOW
        int64_t result;
        if (__builtin_sub_overflow(a, b, &result)) {
            throw errors::RuntimeException("Integer overflow in subtraction");
        }
        return result;
#else
        // Pre-check: would a - b leave the int64 range?
        if ((b < 0 && a > INT64_MAX + b) ||
            (b > 0 && a < INT64_MIN + b)) {
            throw errors::RuntimeException("Integer overflow in subtraction");
        }
        return a - b;
#endif
    }

    inline int64_t checkedMul64(int64_t a, int64_t b) {
#if MTYPE_HAS_BUILTIN_OVERFLOW
        int64_t result;
        if (__builtin_mul_overflow(a, b, &result)) {
            throw errors::RuntimeException("Integer overflow in multiplication");
        }
        return result;
#else
        // Promote to a wider check via 128-bit on MSVC if available, but
        // standard C++ has no portable int128. Fall back to range checks.
        if (a == 0 || b == 0) {
            return 0;
        }
        // Catch the canonical UB case first.
        if (a == INT64_MIN || b == INT64_MIN) {
            // INT64_MIN * anything except 0 or 1 overflows.
            if (!(a == 1 || b == 1)) {
                throw errors::RuntimeException("Integer overflow in multiplication");
            }
            return a * b;
        }
        // Now both operands are in (INT64_MIN, INT64_MAX] (excluding MIN).
        // Use absolute values for the magnitude check; safe because we've
        // ruled out INT64_MIN.
        int64_t absA = a < 0 ? -a : a;
        int64_t absB = b < 0 ? -b : b;
        if (absA > INT64_MAX / absB) {
            throw errors::RuntimeException("Integer overflow in multiplication");
        }
        return a * b;
#endif
    }

    /**
     * Negate with overflow detection. Catches the canonical case of
     * -INT64_MIN, which is undefined behavior because INT64_MAX is one less
     * than the absolute value of INT64_MIN.
     */
    inline int64_t checkedNeg64(int64_t a) {
        if (a == INT64_MIN) {
            throw errors::RuntimeException("Integer overflow in negation");
        }
        return -a;
    }
}
