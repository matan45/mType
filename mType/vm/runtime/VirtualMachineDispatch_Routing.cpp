#include "VirtualMachine.hpp"
#include <cstddef>
#include <cstdint>
#include "executors/ArrayExecutor.hpp"
#include "executors/InlineCacheExecutor.hpp"
#include "executors/ObjectExecutor.hpp"
#include "executors/PrimitiveMethodExecutor.hpp"
#include "executors/VariableExecutor.hpp"
#include "../../errors/RuntimeException.hpp"

namespace vm::runtime
{
    void VirtualMachine::dispatchExtendedOps(const bytecode::BytecodeProgram::Instruction& instr)
    {
        using OpCode = bytecode::OpCode;
        const auto* activeProgram = executionCtx ? executionCtx->program : program;

        switch (instr.opcode)
        {
        // Objects
        case OpCode::NEW_OBJECT: objectExecutor->handleNewObject(instr);
            break;
        case OpCode::NEW_STACK: objectExecutor->handleNewStack(instr);
            break;
        case OpCode::NEW_VALUE_OBJECT: objectExecutor->handleNewValueObject(instr);
            break;
        case OpCode::OBJECT_TO_VALUE: objectExecutor->handleObjectToValue(instr);
            break;
        case OpCode::GET_FIELD:
            if (icEnabled && inlineCacheExecutor)
                inlineCacheExecutor->handleGetFieldIC(instr);
            else
                objectExecutor->handleGetField(instr);
            break;
        case OpCode::GET_FIELD_TYPED:
            // MYT-212: class-targeted field read. IC-bypass — the static type
            // hint is the optimisation; no shape cache layered on top.
            objectExecutor->handleGetFieldTyped(instr);
            break;
        case OpCode::SET_FIELD_TYPED:
            objectExecutor->handleSetFieldTyped(instr);
            break;
        // MYT-274 Phase 2: structural-equality fused opcodes for synthesized
        // hashCode/equals on int-only classes — see OpCode.hpp.
        case OpCode::STRUCT_HASH_INT:
            objectExecutor->handleStructHashInt(instr);
            break;
        case OpCode::STRUCT_EQ_INT:
            objectExecutor->handleStructEqInt(instr);
            break;
        case OpCode::GET_FIELD_CACHED:
        {
            // MYT-194: promoted from GET_FIELD once the IC stabilized. Reachable
            // only with IC enabled; fall back to the generic handler otherwise.
            if (icEnabled && inlineCacheExecutor)
            {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                if (!state)
                    throw errors::RuntimeException("GET_FIELD_CACHED dispatched without side-table entry");
                inlineCacheExecutor->handleGetFieldCached(instr, *state);
            }
            else
                objectExecutor->handleGetField(instr);
            break;
        }
        case OpCode::LOAD_LOCAL_GET_FIELD_CACHED:
        {
            // MYT-198: fused LOAD_LOCAL + GET_FIELD_CACHED. IC-only; if IC is
            // disabled fall back by materialising the LOAD_LOCAL and running
            // the generic GET_FIELD. state.fusedSlot carries the receiver slot.
            if (icEnabled && inlineCacheExecutor)
            {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                if (!state)
                    throw errors::RuntimeException("LOAD_LOCAL_GET_FIELD_CACHED dispatched without side-table entry");
                inlineCacheExecutor->handleLoadLocalGetFieldCached(instr, *state);
            }
            else {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                uint64_t slot = state ? static_cast<uint64_t>(state->fusedSlot) : 0;
                variableExecutor->handleLoadLocal(
                    bytecode::BytecodeProgram::Instruction(
                        bytecode::OpCode::LOAD_LOCAL, slot));
                objectExecutor->handleGetField(instr);
            }
            break;
        }
        case OpCode::SET_FIELD:
            if (icEnabled && inlineCacheExecutor)
                inlineCacheExecutor->handleSetFieldIC(instr);
            else
                objectExecutor->handleSetField(instr);
            break;
        case OpCode::SET_FIELD_CACHED:
        {
            // MYT-194: see GET_FIELD_CACHED above.
            if (icEnabled && inlineCacheExecutor)
            {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                if (!state)
                    throw errors::RuntimeException("SET_FIELD_CACHED dispatched without side-table entry");
                inlineCacheExecutor->handleSetFieldCached(instr, *state);
            }
            else
                objectExecutor->handleSetField(instr);
            break;
        }
        case OpCode::INLINE_SET_FIELD:
            if (icEnabled && inlineCacheExecutor)
                inlineCacheExecutor->handleInlineSetFieldIC(instr);
            else
                objectExecutor->handleInlineSetField(instr);
            break;
        case OpCode::INLINE_GET_FIELD:
            if (icEnabled && inlineCacheExecutor)
                inlineCacheExecutor->handleInlineGetFieldIC(instr);
            else
                objectExecutor->handleInlineGetField(instr);
            break;
        case OpCode::GET_STATIC: objectExecutor->handleGetStatic(instr);
            break;
        case OpCode::SET_STATIC: objectExecutor->handleSetStatic(instr);
            break;
        case OpCode::CALL_METHOD:
        case OpCode::INVOKE:
            if (icEnabled && inlineCacheExecutor)
                inlineCacheExecutor->handleCallMethodIC(instr);
            else
                objectExecutor->handleCallMethod(instr);
            break;
        case OpCode::CALL_METHOD_CACHED:
        {
            // MYT-173: promoted from CALL_METHOD once the IC stabilized. IC-only;
            // fall back to the generic handler when IC is disabled.
            if (icEnabled && inlineCacheExecutor)
            {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                if (!state)
                    throw errors::RuntimeException("CALL_METHOD_CACHED dispatched without side-table entry");
                inlineCacheExecutor->handleCallMethodCached(instr, *state);
            }
            else
                objectExecutor->handleCallMethod(instr);
            break;
        }
        case OpCode::CALL_METHOD_POLY_CACHED:
        {
            // MYT-203: promoted from CALL_METHOD once the IC reaches POLY
            // (2-4 entries). Same IC-only constraint as CALL_METHOD_CACHED.
            if (icEnabled && inlineCacheExecutor)
            {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                if (!state)
                    throw errors::RuntimeException("CALL_METHOD_POLY_CACHED dispatched without side-table entry");
                inlineCacheExecutor->handleCallMethodPolyCached(instr, *state);
            }
            else
                objectExecutor->handleCallMethod(instr);
            break;
        }
        case OpCode::LOAD_LOCAL_CALL_CACHED:
        {
            // MYT-198: fused LOAD_LOCAL + CALL_METHOD_CACHED. state.fusedSlot
            // carries the receiver slot the NOPed LOAD_LOCAL would have pushed.
            if (icEnabled && inlineCacheExecutor)
            {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                if (!state)
                    throw errors::RuntimeException("LOAD_LOCAL_CALL_CACHED dispatched without side-table entry");
                inlineCacheExecutor->handleLoadLocalCallCached(instr, *state);
            }
            else {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                uint64_t slot = state ? static_cast<uint64_t>(state->fusedSlot) : 0;
                variableExecutor->handleLoadLocal(
                    bytecode::BytecodeProgram::Instruction(
                        bytecode::OpCode::LOAD_LOCAL, slot));
                objectExecutor->handleCallMethod(instr);
            }
            break;
        }
        case OpCode::LOAD_LOCAL_CALL_POLY_CACHED:
        {
            // MYT-203: fused LOAD_LOCAL + CALL_METHOD_POLY_CACHED. Symmetric to
            // LOAD_LOCAL_CALL_CACHED above.
            const auto* state = activeProgram->findCachedState(instructionPointer);
            if (icEnabled && inlineCacheExecutor)
            {
                if (!state)
                    throw errors::RuntimeException("LOAD_LOCAL_CALL_POLY_CACHED dispatched without side-table entry");
                inlineCacheExecutor->handleLoadLocalCallPolyCached(instr, *state);
            }
            else {
                // IC disabled: replay the fused pair via the generic handlers.
                // Defaulted slot if side-table vanished — the generic
                // CALL_METHOD will fail with the right error if the receiver
                // isn't where expected.
                uint64_t slot = state ? static_cast<uint64_t>(state->fusedSlot) : 0;
                variableExecutor->handleLoadLocal(
                    bytecode::BytecodeProgram::Instruction(
                        bytecode::OpCode::LOAD_LOCAL, slot));
                objectExecutor->handleCallMethod(instr);
            }
            break;
        }
        case OpCode::SUPER_CONSTRUCTOR: objectExecutor->handleSuperConstructor(instr);
            break;
        case OpCode::THIS_CONSTRUCTOR: objectExecutor->handleThisConstructor(instr);
            break;
        case OpCode::SUPER_INVOKE: objectExecutor->handleSuperInvoke(instr);
            break;
        case OpCode::SUPER_GET_FIELD: objectExecutor->handleSuperGetField(instr);
            break;
        case OpCode::SUPER_SET_FIELD: objectExecutor->handleSuperSetField(instr);
            break;

        // Iterator
        case OpCode::GET_ITERATOR: objectExecutor->handleGetIterator(instr);
            break;
        case OpCode::ITERATOR_HAS_NEXT: objectExecutor->handleIteratorHasNext(instr);
            break;
        case OpCode::ITERATOR_NEXT: objectExecutor->handleIteratorNext(instr);
            break;
        case OpCode::ITERATOR_CLOSE: objectExecutor->handleIteratorClose(instr);
            break;

        // Primitive method optimizations (Phase 3) — Int
        case OpCode::INVOKE_INT_ADD: primitiveMethodExecutor->handleInvokeIntAdd();
            break;
        case OpCode::INVOKE_INT_SUB: primitiveMethodExecutor->handleInvokeIntSub();
            break;
        case OpCode::INVOKE_INT_MUL: primitiveMethodExecutor->handleInvokeIntMul();
            break;
        case OpCode::INVOKE_INT_DIV: primitiveMethodExecutor->handleInvokeIntDiv();
            break;
        case OpCode::INVOKE_INT_MOD: primitiveMethodExecutor->handleInvokeIntMod();
            break;
        case OpCode::INVOKE_INT_NEG: primitiveMethodExecutor->handleInvokeIntNeg();
            break;
        case OpCode::INVOKE_INT_ABS: primitiveMethodExecutor->handleInvokeIntAbs();
            break;
        case OpCode::INVOKE_INT_EQUALS: primitiveMethodExecutor->handleInvokeIntEquals();
            break;
        case OpCode::INVOKE_INT_COMPARE: primitiveMethodExecutor->handleInvokeIntCompare();
            break;
        case OpCode::INVOKE_INT_GET_VALUE: primitiveMethodExecutor->handleInvokeIntGetValue();
            break;
        case OpCode::INVOKE_INT_LESS_THAN: primitiveMethodExecutor->handleInvokeIntLessThan();
            break;
        case OpCode::INVOKE_INT_LESS_EQUAL: primitiveMethodExecutor->handleInvokeIntLessEqual();
            break;
        case OpCode::INVOKE_INT_GREATER_THAN: primitiveMethodExecutor->handleInvokeIntGreaterThan();
            break;
        case OpCode::INVOKE_INT_GREATER_EQUAL: primitiveMethodExecutor->handleInvokeIntGreaterEqual();
            break;

        // Float
        case OpCode::INVOKE_FLOAT_ADD: primitiveMethodExecutor->handleInvokeFloatAdd();
            break;
        case OpCode::INVOKE_FLOAT_SUB: primitiveMethodExecutor->handleInvokeFloatSub();
            break;
        case OpCode::INVOKE_FLOAT_MUL: primitiveMethodExecutor->handleInvokeFloatMul();
            break;
        case OpCode::INVOKE_FLOAT_DIV: primitiveMethodExecutor->handleInvokeFloatDiv();
            break;
        case OpCode::INVOKE_FLOAT_NEG: primitiveMethodExecutor->handleInvokeFloatNeg();
            break;
        case OpCode::INVOKE_FLOAT_ABS: primitiveMethodExecutor->handleInvokeFloatAbs();
            break;
        case OpCode::INVOKE_FLOAT_EQUALS: primitiveMethodExecutor->handleInvokeFloatEquals();
            break;
        case OpCode::INVOKE_FLOAT_COMPARE: primitiveMethodExecutor->handleInvokeFloatCompare();
            break;
        case OpCode::INVOKE_FLOAT_GET_VALUE: primitiveMethodExecutor->handleInvokeFloatGetValue();
            break;
        case OpCode::INVOKE_BOOL_GET_VALUE: primitiveMethodExecutor->handleInvokeBoolGetValue();
            break;
        case OpCode::INVOKE_FLOAT_LESS_THAN: primitiveMethodExecutor->handleInvokeFloatLessThan();
            break;
        case OpCode::INVOKE_FLOAT_LESS_EQUAL: primitiveMethodExecutor->handleInvokeFloatLessEqual();
            break;
        case OpCode::INVOKE_FLOAT_GREATER_THAN: primitiveMethodExecutor->handleInvokeFloatGreaterThan();
            break;
        case OpCode::INVOKE_FLOAT_GREATER_EQUAL: primitiveMethodExecutor->handleInvokeFloatGreaterEqual();
            break;

        // Bool
        case OpCode::INVOKE_BOOL_AND: primitiveMethodExecutor->handleInvokeBoolAnd();
            break;
        case OpCode::INVOKE_BOOL_OR: primitiveMethodExecutor->handleInvokeBoolOr();
            break;
        case OpCode::INVOKE_BOOL_XOR: primitiveMethodExecutor->handleInvokeBoolXor();
            break;
        case OpCode::INVOKE_BOOL_NOT: primitiveMethodExecutor->handleInvokeBoolNot();
            break;
        case OpCode::INVOKE_BOOL_EQUALS: primitiveMethodExecutor->handleInvokeBoolEquals();
            break;

        // String
        case OpCode::INVOKE_STRING_LENGTH: primitiveMethodExecutor->handleInvokeStringLength();
            break;
        case OpCode::INVOKE_STRING_CONCAT: primitiveMethodExecutor->handleInvokeStringConcat();
            break;
        case OpCode::INVOKE_STRING_EQUALS: primitiveMethodExecutor->handleInvokeStringEquals();
            break;
        case OpCode::INVOKE_STRING_IS_EMPTY: primitiveMethodExecutor->handleInvokeStringIsEmpty();
            break;

        // Arrays
        case OpCode::NEW_ARRAY: arrayExecutor->handleNewArray(instr);
            break;
        case OpCode::NEW_ARRAY_MULTI: arrayExecutor->handleNewArrayMulti(instr);
            break;
        case OpCode::ARRAY_GET: arrayExecutor->handleArrayGet();
            break;
        case OpCode::ARRAY_GET_ALIAS: arrayExecutor->handleArrayGetAlias();
            break;
        case OpCode::ARRAY_SET: arrayExecutor->handleArraySet();
            break;
        case OpCode::ARRAY_LENGTH: arrayExecutor->handleArrayLength();
            break;
        case OpCode::ARRAY_GET_FIELD: arrayExecutor->handleArrayGetField(instr);
            break;
        case OpCode::ARRAY_SET_FIELD: arrayExecutor->handleArraySetField(instr);
            break;
        case OpCode::ARRAY_GET_INT_LOCAL: arrayExecutor->handleArrayGetIntLocal(instr);
            break;
        case OpCode::ARRAY_SET_INT_LOCAL: arrayExecutor->handleArraySetIntLocal(instr);
            break;
        case OpCode::ARRAY_LENGTH_LOCAL: arrayExecutor->handleArrayLengthLocal(instr);
            break;

        default:
            dispatchSpecialOps(instr);
            break;
        }
    }
}
