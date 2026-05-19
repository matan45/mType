#include "JitEmissionState.hpp"
#include "JitCompiler_Objects.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>
#include <cstdint>

namespace vm::jit
{
    using OpCode = bytecode::OpCode;

    bool emitObjectOps(JitEmissionState& s,
                       const bytecode::BytecodeProgram::Instruction& instr)
    {
        // Object/method emitters cross many cc.invoke sites and read stackBase
        // memory directly; they're not hint-aware. Flush at entry.
        flushAllHints(s);

        if (emitArrayOps(s, instr)) return true;

        switch (instr.opcode)
        {
            case OpCode::PUSH_STRING:    return emitPushStringOp(s, instr);
            case OpCode::GET_FIELD:
            case OpCode::GET_FIELD_CACHED:
                // CACHED routes through the same emit path as GET_FIELD.
                // tryEmitInlinedFieldGet consults the IC by IP, independent
                // of opcode variant.
                return emitGetFieldOp(s, instr);
            case OpCode::LOAD_LOCAL_GET_FIELD_CACHED:
            {
                // De-fuse at JIT time. Emit LOAD_LOCAL(fusedSlot) followed by
                // the generic GET_FIELD. Fused state lives on the
                // BytecodeProgram side table keyed by currentIP.
                const auto* state = s.program.findCachedState(s.currentIP);
                uint64_t slot = state ? static_cast<uint64_t>(state->fusedSlot) : 0;
                bytecode::BytecodeProgram::Instruction loadLocal(
                    OpCode::LOAD_LOCAL, slot);
                if (!emitControlFlowOps(s, loadLocal, nullptr))
                {
                    s.compileFailed = true;
                    return true;
                }
                return emitGetFieldOp(s, instr);
            }
            case OpCode::SET_FIELD:
            case OpCode::SET_FIELD_CACHED:
                return emitSetFieldOp(s, instr);
            case OpCode::GET_FIELD_TYPED:
            {
                // Typed-receiver field read. For value classes (no
                // inheritance) and reference classes without field shadowing,
                // the un-typed runtime-class lookup produces the same slot as
                // the typed static-binding lookup. Field-shadowing across a
                // hierarchy diverges; the interpreter path retains strict
                // static-binding semantics. Dedicated JIT emit that resolves
                // the owner's slot at compile time is a follow-up.
                bytecode::BytecodeProgram::Instruction getField(
                    OpCode::GET_FIELD, instr.inlineOperands[1]);
                return emitGetFieldOp(s, getField);
            }
            case OpCode::SET_FIELD_TYPED:
            {
                bytecode::BytecodeProgram::Instruction setField(
                    OpCode::SET_FIELD, instr.inlineOperands[1]);
                return emitSetFieldOp(s, setField);
            }
            case OpCode::INLINE_GET_FIELD: return emitGetFieldOp(s, instr);
            case OpCode::INLINE_SET_FIELD: return emitSetFieldOp(s, instr);
            case OpCode::CALL_STATIC:    return emitCallStaticOp(s, instr);
            case OpCode::CALL_METHOD:
            case OpCode::CALL_METHOD_CACHED:
            case OpCode::CALL_METHOD_POLY_CACHED:
                // CACHED + POLY_CACHED route through CALL_METHOD's emitter —
                // tryEmitInlinedMethodCall reads the IC (unaffected by opcode
                // rewrite). Mixed-inlineability POLY sites inline the
                // eligible shapes and route the rest through jit_call_method_ic.
                return emitCallMethodOp(s, instr);
            case OpCode::LOAD_LOCAL_CALL_CACHED:
            {
                const auto* state = s.program.findCachedState(s.currentIP);
                uint64_t slot = state ? static_cast<uint64_t>(state->fusedSlot) : 0;
                bytecode::BytecodeProgram::Instruction loadLocal(
                    OpCode::LOAD_LOCAL, slot);
                if (!emitControlFlowOps(s, loadLocal, nullptr))
                {
                    s.compileFailed = true;
                    return true;
                }
                return emitCallMethodOp(s, instr);
            }
            case OpCode::LOAD_LOCAL_CALL_POLY_CACHED:
            {
                const auto* state = s.program.findCachedState(s.currentIP);
                uint64_t slot = state ? static_cast<uint64_t>(state->fusedSlot) : 0;
                bytecode::BytecodeProgram::Instruction loadLocal(
                    OpCode::LOAD_LOCAL, slot);
                if (!emitControlFlowOps(s, loadLocal, nullptr))
                {
                    s.compileFailed = true;
                    return true;
                }
                return emitCallMethodOp(s, instr);
            }
            case OpCode::LOAD_GET_FIELD:
            {
                // Compile-time fused LOAD_LOCAL + GET_FIELD. Operands are
                // [slot, field_name_idx]. De-fuse by chaining the existing
                // emitters — net machine code matches the unfused sequence.
                bytecode::BytecodeProgram::Instruction loadLocal(
                    OpCode::LOAD_LOCAL, instr.inlineOperands[0]);
                if (!emitControlFlowOps(s, loadLocal, nullptr))
                {
                    s.compileFailed = true;
                    return true;
                }
                bytecode::BytecodeProgram::Instruction getField(
                    OpCode::GET_FIELD, instr.inlineOperands[1]);
                return emitGetFieldOp(s, getField);
            }
            case OpCode::INSTANCEOF:     return emitInstanceofOp(s, instr);
            case OpCode::INSTANCEOF_TYPEPARAM: return emitInstanceofTypeParamOp(s, instr);
            case OpCode::CAST:           return emitCastOp(s, instr);
            case OpCode::CAST_TYPEPARAM: return emitCastTypeParamOp(s, instr);
            case OpCode::BIND_TYPE_ARGS: return emitBindTypeArgsOp(s, instr);
            case OpCode::NEW_OBJECT:
            case OpCode::NEW_VALUE_OBJECT: return emitNewObjectOp(s, instr);
            case OpCode::NEW_STACK: return emitNewStackOp(s, instr);
            case OpCode::CREATE_PROMISE:
                return emitCreatePromiseOp(s);
            case OpCode::OBJECT_TO_VALUE_CREATE_PROMISE:
                return emitObjectToValueCreatePromiseOp(s);
            case OpCode::CREATE_PROMISE_RETURN_VALUE:
            {
                if (!emitCreatePromiseOp(s))
                    return false;
                bytecode::BytecodeProgram::Instruction ret(OpCode::RETURN_VALUE);
                return emitControlFlowOps(s, ret, nullptr);
            }
            case OpCode::AWAIT:
                return emitAwaitOp(s);

            case OpCode::GET_ITERATOR:       return emitGetIteratorOp(s);
            case OpCode::ITERATOR_HAS_NEXT:  return emitIteratorHasNextOp(s);
            case OpCode::ITERATOR_NEXT:      return emitIteratorNextOp(s);
            case OpCode::ITERATOR_CLOSE:     return emitIteratorCloseOp(s);

            case OpCode::OBJECT_TO_VALUE:    return emitObjectToValueOp(s);

            case OpCode::STRUCT_HASH_INT:    return emitStructHashIntOp(s, instr);
            case OpCode::STRUCT_EQ_INT:      return emitStructEqIntOp(s, instr);

            default:
                return false;
        }
    }
}
