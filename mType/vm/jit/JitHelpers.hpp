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

        // --- Phase 4: Boxed value lifecycle (non-throwing) ---

        // Placement-new monostate into count consecutive Value slots
        void jit_locals_init(value::Value* base, size_t count);

        // Destroy count consecutive Value slots (calls destructors)
        void jit_locals_cleanup(value::Value* base, size_t count);

        // Destroy a single Value slot and reinitialize as monostate
        void jit_value_destroy(value::Value* dest);
    }

    // --- Phase 3: Call dispatch ---
    // Not extern "C" because these may throw C++ exceptions.

    void jit_call_function(JitContext* ctx, uint32_t nameIndex, size_t argCount);

    // --- Phase 3: Generic arithmetic helpers ---
    void jit_generic_add(value::Value* result, const value::Value* left, const value::Value* right);
    void jit_generic_sub(value::Value* result, const value::Value* left, const value::Value* right);
    void jit_generic_mul(value::Value* result, const value::Value* left, const value::Value* right);
    void jit_generic_div(value::Value* result, const value::Value* left, const value::Value* right);
    void jit_generic_mod(value::Value* result, const value::Value* left, const value::Value* right);

    // Not extern "C" because it throws a C++ exception.
    void jit_throw_div_by_zero();

    // --- Phase 4: Boxed value helpers (may allocate/throw) ---

    // Copy a Value from src to dest (uses variant assignment, safe for overwrite)
    void jit_value_copy(value::Value* dest, const value::Value* src);

    // Store a boxed Value as the function return value
    void jit_set_return_boxed(JitContext* ctx, const value::Value* val);

    // Swap two Values
    void jit_value_swap(value::Value* a, value::Value* b);

    // --- Phase 4A: String support ---

    // Intern a string from the constant pool and store in dest
    void jit_push_string(value::Value* dest,
                          const vm::bytecode::BytecodeProgram* prog,
                          uint32_t constIndex);

    // --- Phase 4B: Object field access ---

    // Get field value from object, store in dest
    void jit_get_field(value::Value* dest, const value::Value* object,
                        const vm::bytecode::BytecodeProgram* prog,
                        uint32_t fieldNameIndex, uint8_t flags = 0);

    // Set field on object, copy newValue to destValue (for assignment chaining)
    void jit_set_field(value::Value* destValue, const value::Value* object,
                        const value::Value* newValue,
                        const vm::bytecode::BytecodeProgram* prog,
                        uint32_t fieldNameIndex, uint8_t flags = 0);

    // --- Phase 4C: Array operations ---

    // Create a new array and store in dest
    void jit_new_array(value::Value* dest, JitContext* ctx,
                        uint32_t typeIndex, int64_t size);

    // Get element at index from array, store in dest
    void jit_array_get(value::Value* dest, const value::Value* array,
                        int64_t index);

    // Set element at index in array
    void jit_array_set(const value::Value* array, int64_t index,
                        const value::Value* newValue);

    // Get array length
    int64_t jit_array_length(const value::Value* array);

    // --- Phase 4D: Method calls ---

    // Dispatch a static method call (qualified name already in constant pool)
    void jit_call_static(JitContext* ctx, uint32_t nameIndex, size_t argCount);

    // Dispatch an instance method call
    // Object is in ctx->callArgs[0], args in callArgs[1..argCount]
    void jit_call_method(JitContext* ctx, uint32_t methodNameIndex, size_t argCount);

    // --- Phase 4E: Type operations ---

    // Check if value is an instance of the given type. Returns 1 or 0.
    int64_t jit_instanceof(const value::Value* val,
                            const vm::bytecode::BytecodeProgram* prog,
                            uint32_t typeIndex);

    // Cast value to target type, store result in dest
    void jit_cast(value::Value* dest, const value::Value* src,
                   const vm::bytecode::BytecodeProgram* prog,
                   uint32_t typeIndex);

    // Create a new object instance with constructor args from ctx->callArgs
    void jit_new_object(value::Value* dest, JitContext* ctx,
                         uint32_t classIndex, size_t argCount);

    // Convert ObjectInstance on stack to lightweight ValueObject
    void jit_object_to_value(value::Value* val);

    // --- Phase 5: OSR helpers ---

    // Write a local Value to the OSR output buffer
    extern "C" {
        void jit_osr_write_local(JitContext* ctx, size_t slot, const value::Value* val);
        void jit_osr_exit(JitContext* ctx, uint64_t exitOffset);
    }

    // Trigger deoptimization from JIT code (throws OSRDeoptException)
    void jit_osr_deoptimize(JitContext* ctx, uint64_t bytecodeOffset);

    // --- Phase 7: IC-aware field access ---

    void jit_get_field_ic(value::Value* dest, const value::Value* object,
                          JitContext* ctx, size_t bytecodeOffset,
                          uint32_t fieldNameIndex, uint8_t flags = 0);

    void jit_set_field_ic(value::Value* destValue, const value::Value* object,
                          const value::Value* newValue,
                          JitContext* ctx, size_t bytecodeOffset,
                          uint32_t fieldNameIndex, uint8_t flags = 0);

    // --- Phase 7: Typed array access ---

    int64_t jit_array_get_int(const value::Value* array, int64_t index);
    void jit_array_set_int(const value::Value* array, int64_t index,
                           int64_t val);

    // --- Phase 7: SoA-aware array field access ---

    void jit_array_get_field(value::Value* dest, const value::Value* array,
                             int64_t index,
                             const vm::bytecode::BytecodeProgram* prog,
                             uint32_t fieldNameIndex);

    void jit_array_set_field(const value::Value* array, int64_t index,
                             const value::Value* newValue,
                             const vm::bytecode::BytecodeProgram* prog,
                             uint32_t fieldNameIndex);
}
