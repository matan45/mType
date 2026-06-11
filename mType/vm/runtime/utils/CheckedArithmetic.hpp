#pragma once

#include <cstdint>
#include <climits>
#include <string>
#include "../../../errors/RuntimeException.hpp"

namespace vm::runtime::utils
{
    /**
     * int64 arithmetic helpers — two flavors:
     *
     * 1. wrappingAdd64 / wrappingSub64 / wrappingMul64 / wrappingNeg64
     *    Default semantics for the VM's `+`, `-`, `*`, `INC`, `DEC`, `NEG`
     *    instructions. Casts through uint64_t so the operation is
     *    well-defined two's-complement wrap (no UB). This matches Java /
     *    C# (unchecked) / Go / Rust (`wrapping_*`) and is the only
     *    semantics that doesn't break legitimate hash, CRC, RNG, and
     *    bit-twiddling code that intentionally wraps.
     *
     * 2. checkedAdd64 / checkedSub64 / checkedMul64 / checkedNeg64
     *    Throwing variants. Available for future Math.addExact-style APIs
     *    where the script explicitly opts into overflow detection. NOT
     *    used by the default arithmetic opcodes.
     *
     * The point of MYT-29 here is to eliminate undefined behavior, not to
     * change language semantics. Wrapping covers the UB without changing
     * observable behavior for normal code.
     */

    // ---- Wrapping (default) ----

    inline int64_t wrappingAdd64(int64_t a, int64_t b) {
        return static_cast<int64_t>(static_cast<uint64_t>(a) + static_cast<uint64_t>(b));
    }

    inline int64_t wrappingSub64(int64_t a, int64_t b) {
        return static_cast<int64_t>(static_cast<uint64_t>(a) - static_cast<uint64_t>(b));
    }

    inline int64_t wrappingMul64(int64_t a, int64_t b) {
        return static_cast<int64_t>(static_cast<uint64_t>(a) * static_cast<uint64_t>(b));
    }

    inline int64_t wrappingNeg64(int64_t a) {
        // -INT64_MIN wraps to INT64_MIN, matching Java's `-x` semantics.
        return static_cast<int64_t>(0u - static_cast<uint64_t>(a));
    }

    inline int64_t wrappingDiv64(int64_t a, int64_t b) {
        // INT64_MIN / -1 overflows the hardware idiv result. Java defines it
        // as wrapping to INT64_MIN; keep the VM's default arithmetic semantics.
        if (a == INT64_MIN && b == -1) {
            return INT64_MIN;
        }
        return a / b;
    }

    inline int64_t wrappingMod64(int64_t a, int64_t b) {
        // INT64_MIN % -1 is paired with the idiv overflow case above; Java
        // defines the remainder as 0.
        if (a == INT64_MIN && b == -1) {
            return 0;
        }
        return a % b;
    }

    // ---- Checked (opt-in, throws on overflow) ----

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
