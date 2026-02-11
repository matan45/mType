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
        // --- Phase 2: Int/Bool support ---

        // Unbox a Value to int64_t. Handles int64_t and bool.
        int64_t jit_unbox_int(const value::Value* val);

        // Box an int64_t into a Value and store as return value in JitContext.
        void jit_set_return_int(JitContext* ctx, int64_t val);

        // Box a bool into a Value and store as return value in JitContext.
        void jit_set_return_bool(JitContext* ctx, int64_t val);

        // GC safepoint - call at loop back-edges
        void jit_gc_safepoint();

        // --- Phase 3: Float support ---

        // Unbox a Value to float. Handles float and int64_t (promotion).
        float jit_unbox_float(const value::Value* val);

        // Box a float into a Value and store as return value in JitContext.
        void jit_set_return_float(JitContext* ctx, float val);

        // --- Phase 3: Boxing helpers (for CALL argument preparation) ---

        void jit_box_int(value::Value* dest, int64_t val);
        void jit_box_float(value::Value* dest, float val);
        void jit_box_bool(value::Value* dest, int64_t val);
        void jit_box_null(value::Value* dest);
    }

    // --- Phase 3: Call dispatch ---
    // Not extern "C" because these may throw C++ exceptions.

    // Dispatch a function call from JIT code.
    // Checks JIT cache -> native registry -> interpreter fallback.
    // Stores result in ctx->returnValue, sets ctx->hasReturnValue.
    void jit_call_function(JitContext* ctx, uint32_t nameIndex, size_t argCount);

    // --- Phase 3: Generic arithmetic helpers ---

    // These replicate ArithmeticExecutor::performBinaryOp logic:
    // int+int->int, float+float->float, int+float->float (promote)
    void jit_generic_add(value::Value* result, const value::Value* left, const value::Value* right);
    void jit_generic_sub(value::Value* result, const value::Value* left, const value::Value* right);
    void jit_generic_mul(value::Value* result, const value::Value* left, const value::Value* right);
    void jit_generic_div(value::Value* result, const value::Value* left, const value::Value* right);
    void jit_generic_mod(value::Value* result, const value::Value* left, const value::Value* right);

    // Not extern "C" because it throws a C++ exception.
    void jit_throw_div_by_zero();
}
