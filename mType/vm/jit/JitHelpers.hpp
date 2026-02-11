#pragma once
#include "JitContext.hpp"
#include "../../value/ValueType.hpp"
#include <cstdint>

namespace vm::jit
{
    /**
     * C-linkage helper functions callable from JIT-compiled code.
     * These handle the Value variant manipulation at JIT boundaries,
     * keeping the hot path free from variant overhead.
     */
    extern "C" {
        // Unbox a Value to int64_t. Assumes the Value holds int64_t.
        int64_t jit_unbox_int(const value::Value* val);

        // Box an int64_t into a Value and store it as the return value in JitContext.
        void jit_set_return_int(JitContext* ctx, int64_t val);

        // Box a bool into a Value and store it as the return value in JitContext.
        void jit_set_return_bool(JitContext* ctx, int64_t val);

        // GC safepoint — call at loop back-edges
        void jit_gc_safepoint();
    }

    // Not extern "C" because it throws a C++ exception.
    // MSVC treats extern "C" functions as implicitly noexcept.
    void jit_throw_div_by_zero();
}
