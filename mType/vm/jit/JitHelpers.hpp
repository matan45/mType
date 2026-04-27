#pragma once
#include "JitContext.hpp"
#include "../../value/ValueType.hpp"
#include <cstdint>

// Forward declaration
namespace vm::bytecode {
    class BytecodeProgram;
}

namespace vm::jit
{
    // GC safepoint polling counter. JIT-emitted code (JUMP_BACK back-edge,
    // self-recursive tail call) inlines the inc + threshold check and only
    // invokes jit_gc_safepoint() once per gc::config::GC_CHECK_INTERVAL
    // crossings.
    //
    // THREADING INVARIANT: plain size_t, not thread_local / not std::atomic.
    // This is load-bearing on the VM being single-threaded — async/await in
    // mType runs on the same OS thread via an event loop (see AsyncPromise
    // and VirtualMachine::driveAsyncInvocation). If the VM ever grows
    // multi-threaded (worker threads sharing a single VirtualMachine
    // instance, true parallel JIT execution, etc.), this counter MUST become
    // either:
    //   - thread_local (simplest, pays TLS access cost per JIT back-edge) or
    //   - std::atomic<size_t> with relaxed ordering (benign race on the
    //     threshold crossing is acceptable for GC polling cadence).
    // The current single-threaded assumption is what makes the plain-size_t
    // choice safe; maybeCollect() itself is internally thread-safe via its
    // own atomic flags, so a benign race on the counter only shifts polling
    // cadence — but a race on the store-back could lose an increment and
    // never trigger the threshold on a pathological schedule.
    extern size_t g_jit_gc_poll_counter;

    /**
     * C-linkage helper functions callable from JIT-compiled code.
     * These handle the Value variant manipulation at JIT boundaries,
     * keeping the hot path free from variant overhead.
     */
    extern "C" {
        int64_t jit_unbox_int(const value::Value* val);
        void jit_set_return_int(JitContext* ctx, int64_t val);
        void jit_set_return_bool(JitContext* ctx, int64_t val);
        void jit_gc_safepoint();

        double jit_unbox_float(const value::Value* val);
        void jit_set_return_float(JitContext* ctx, double val);

        void jit_box_int(value::Value* dest, int64_t val);
        void jit_box_float(value::Value* dest, double val);
        void jit_box_bool(value::Value* dest, int64_t val);
        void jit_box_null(value::Value* dest);

        void jit_locals_init(value::Value* base, size_t count);
        void jit_locals_cleanup(value::Value* base, size_t count);
        void jit_value_destroy(value::Value* dest);
        int64_t jit_values_equal(const value::Value* left, const value::Value* right);
    }

    void jit_call_function(JitContext* ctx, uint32_t nameIndex, size_t argCount);
    void jit_call_function_fast(JitContext* ctx, uint32_t funcIndex, size_t argCount);

    void jit_generic_add(value::Value* result, const value::Value* left, const value::Value* right);
    void jit_generic_sub(value::Value* result, const value::Value* left, const value::Value* right);
    void jit_generic_mul(value::Value* result, const value::Value* left, const value::Value* right);
    void jit_generic_div(value::Value* result, const value::Value* left, const value::Value* right);
    void jit_generic_mod(value::Value* result, const value::Value* left, const value::Value* right);

    void jit_throw_div_by_zero();
    void jit_throw_shift_out_of_range(int64_t count);
    void jit_throw_array_oob(int64_t index, int64_t size);

    void jit_value_copy(value::Value* dest, const value::Value* src);
    void jit_set_return_boxed(JitContext* ctx, const value::Value* val);
    void jit_value_swap(value::Value* a, value::Value* b);

    void jit_push_string(value::Value* dest,
                          const vm::bytecode::BytecodeProgram* prog,
                          uint32_t constIndex);

    void jit_new_array(value::Value* dest, JitContext* ctx,
                        uint32_t typeIndex, int64_t size);
    void jit_new_array_multi(value::Value* dest, JitContext* ctx,
                              uint32_t typeIndex,
                              uint32_t totalDims,
                              uint32_t specifiedDims);
    void jit_array_get(value::Value* dest, const value::Value* array,
                        int64_t index);
    void jit_array_set(const value::Value* array, int64_t index,
                        const value::Value* newValue);
    int64_t jit_array_length(const value::Value* array);

    void jit_call_method(JitContext* ctx, uint32_t methodNameIndex, size_t argCount);

    // MYT-163: extract the ClassDefinition* from a receiver Value. Used by the
    // speculative-inlining shape guard. Returns the raw ClassDefinition pointer
    // for an ObjectInstance receiver; returns nullptr for any other variant
    // (ValueObject, primitive, null, array, lambda, ...). Nullptr always
    // mismatches the cached shape and triggers the fallback path, which is the
    // correct behaviour — a mid-function receiver-kind change falls through to
    // the generic jit_call_method_ic.
    const void* jit_extract_classdef(const value::Value* receiver);

    // MYT-172 AC #3: returns a pointer to the receiver's field-vector data
    // (Value[N]) for use by the JIT inline field-IC GET fast path. Returns
    // nullptr when the receiver isn't an ObjectInstance/ValueObject, when
    // the bridge is null, or when an ObjectInstance hasn't yet had its
    // fieldVector populated (lazy init). Callers branch to the slow-path
    // helper on null. Bypassing the IC table & name resolution is the
    // entire point — this helper does no map lookups.
    const value::Value* jit_field_data_const(const value::Value* receiver) noexcept;

    // MYT-191: thin inline-IC SET helper. Writes *newValue into the
    // ObjectInstance receiver's field[fieldIndex] using the write-barrier-
    // correct setFieldByIndex path, then mirrors *destSlot = *newValue so
    // the destination stack slot holds the assigned value (matching the
    // semantics of jit_set_field_ic). Returns false on any bail condition
    // (null pointers, non-ObjectInstance receiver including ValueObject, or
    // out-of-range fieldIndex) so the caller can jump to the slow path
    // which handles ValueObject CoW and IC table maintenance.
    bool jit_field_set_at(value::Value* destSlot,
                          const value::Value* receiverSlot,
                          size_t fieldIndex,
                          const value::Value* newValue) noexcept;

    // CALL_METHOD with inline-cache fast path. Mirrors emitGetFieldOp/jit_get_field_ic
    // pattern: emitter passes the bytecode IP, helper looks up MethodInlineCache by
    // offset, on monomorphic/polymorphic shape match dispatches via pre-resolved
    // funcMetadata (skipping name resolution). On miss falls back to jit_call_method
    // and populates the cache from the resolved metadata.
    void jit_call_method_ic(JitContext* ctx,
                             size_t bytecodeOffset,
                             uint32_t methodNameIndex,
                             size_t argCount);

    // MYT-152: global/field variable access from JIT-compiled OSR loops.
    // Mirrors VariableExecutor::handleLoadVar / handleStoreVar including the
    // findVariable -> instance-field -> static-field fallback chain. Throws
    // are caught and stored in ctx->pendingException.
    void jit_load_var(value::Value* dest, JitContext* ctx, uint32_t nameIndex);
    void jit_store_var(JitContext* ctx, uint32_t nameIndex,
                       const value::Value* val);
    // MYT-208: DECLARE_VAR — registers a new global with the value popped
    // from the operand stack. Used in the JIT emitter for the rare-but-real
    // case where a function's metadata range trails into top-level globals.
    void jit_declare_var(JitContext* ctx, uint32_t nameIndex,
                         const value::Value* val, uint8_t isFinal);

    // MYT-147: iterator protocol helpers. Receiver is marshalled into
    // ctx->callArgs[0] by the emitter before each call. Methods that return a
    // value (get/has_next/next) write the result into *dest.
    void jit_iterator_get     (value::Value* dest, JitContext* ctx);
    void jit_iterator_has_next(value::Value* dest, JitContext* ctx);
    void jit_iterator_next    (value::Value* dest, JitContext* ctx);
    void jit_iterator_close   (JitContext* ctx);

    int64_t jit_instanceof(const value::Value* val,
                            const vm::bytecode::BytecodeProgram* prog,
                            uint32_t typeIndex);
    void jit_cast(value::Value* dest, const value::Value* src,
                   const vm::bytecode::BytecodeProgram* prog,
                   uint32_t typeIndex);
    void jit_cast_typeparam(value::Value* dest, const value::Value* src,
                             JitContext* ctx,
                             uint32_t paramNameIndex);
    void jit_new_object(value::Value* dest, JitContext* ctx,
                         uint32_t classIndex, size_t argCount);
    // MYT-208: stack-promoted allocation. Same calling convention as
    // jit_new_object; produces a STACK_OBJECT-tagged Value (or OBJECT if
    // VM-side fallback fired for a non-trivial ctor).
    void jit_new_stack(value::Value* dest, JitContext* ctx,
                        uint32_t classIndex, size_t argCount);
    void jit_object_to_value(value::Value* val);

    extern "C" {
        void jit_osr_write_local(JitContext* ctx, size_t slot, const value::Value* val);
        void jit_osr_exit(JitContext* ctx, uint64_t exitOffset);
    }

    void jit_osr_deoptimize(JitContext* ctx, uint64_t bytecodeOffset);

    void jit_get_field_ic(value::Value* dest, const value::Value* object,
                          JitContext* ctx, size_t bytecodeOffset,
                          uint32_t fieldNameIndex, uint8_t flags = 0);

    void jit_set_field_ic(value::Value* destValue, const value::Value* object,
                          const value::Value* newValue,
                          JitContext* ctx, size_t bytecodeOffset,
                          uint32_t fieldNameIndex, uint8_t flags = 0);

    int64_t jit_array_get_int(const value::Value* array, int64_t index);
    void jit_array_set_int(const value::Value* array, int64_t index,
                           int64_t val);
    int64_t* jit_array_get_raw_int_ptr(const value::Value* array);

    struct JitArrayInfo {
        int64_t* data;     // raw int pointer (nullptr if heterogeneous)
        int64_t  length;   // element count
    };
    void jit_array_extract_info(const value::Value* array, JitArrayInfo* out);

    void jit_array_get_field(value::Value* dest, const value::Value* array,
                             int64_t index,
                             const vm::bytecode::BytecodeProgram* prog,
                             uint32_t fieldNameIndex);

    void jit_array_set_field(const value::Value* array, int64_t index,
                             const value::Value* newValue,
                             const vm::bytecode::BytecodeProgram* prog,
                             uint32_t fieldNameIndex);
}
