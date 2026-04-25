#include "JitCompiler.hpp"
#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>
#include <cstring>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    const std::unordered_set<uint8_t>& JitCompiler::getSupportedOpcodes()
    {
        static const std::unordered_set<uint8_t> supported = {
            static_cast<uint8_t>(OpCode::PUSH_INT),
            static_cast<uint8_t>(OpCode::PUSH_FLOAT),
            static_cast<uint8_t>(OpCode::PUSH_BOOL),
            static_cast<uint8_t>(OpCode::PUSH_NULL),
            static_cast<uint8_t>(OpCode::POP),
            static_cast<uint8_t>(OpCode::DUP),
            static_cast<uint8_t>(OpCode::SWAP),

            static_cast<uint8_t>(OpCode::ADD_INT),
            static_cast<uint8_t>(OpCode::SUB_INT),
            static_cast<uint8_t>(OpCode::MUL_INT),
            static_cast<uint8_t>(OpCode::DIV_INT),
            static_cast<uint8_t>(OpCode::NEG),
            static_cast<uint8_t>(OpCode::INC),
            static_cast<uint8_t>(OpCode::DEC),

            static_cast<uint8_t>(OpCode::ADD_FLOAT),
            static_cast<uint8_t>(OpCode::SUB_FLOAT),
            static_cast<uint8_t>(OpCode::MUL_FLOAT),
            static_cast<uint8_t>(OpCode::DIV_FLOAT),

            static_cast<uint8_t>(OpCode::ADD),
            static_cast<uint8_t>(OpCode::SUB),
            static_cast<uint8_t>(OpCode::MUL),
            static_cast<uint8_t>(OpCode::DIV),
            static_cast<uint8_t>(OpCode::MOD),

            static_cast<uint8_t>(OpCode::EQ),
            static_cast<uint8_t>(OpCode::NE),
            static_cast<uint8_t>(OpCode::LT),
            static_cast<uint8_t>(OpCode::GT),
            static_cast<uint8_t>(OpCode::LE),
            static_cast<uint8_t>(OpCode::GE),
            static_cast<uint8_t>(OpCode::EQ_INT),
            static_cast<uint8_t>(OpCode::NE_INT),
            static_cast<uint8_t>(OpCode::LT_INT),
            static_cast<uint8_t>(OpCode::GT_INT),

            static_cast<uint8_t>(OpCode::NOT),
            static_cast<uint8_t>(OpCode::AND),
            static_cast<uint8_t>(OpCode::OR),

            static_cast<uint8_t>(OpCode::BITWISE_AND_OP),
            static_cast<uint8_t>(OpCode::BITWISE_OR_OP),
            static_cast<uint8_t>(OpCode::BITWISE_XOR_OP),
            static_cast<uint8_t>(OpCode::LEFT_SHIFT_OP),
            static_cast<uint8_t>(OpCode::RIGHT_SHIFT_OP),
            static_cast<uint8_t>(OpCode::BITWISE_NOT_OP),
            static_cast<uint8_t>(OpCode::BITWISE_AND_INT),
            static_cast<uint8_t>(OpCode::BITWISE_OR_INT),
            static_cast<uint8_t>(OpCode::BITWISE_XOR_INT),
            static_cast<uint8_t>(OpCode::LEFT_SHIFT_INT),
            static_cast<uint8_t>(OpCode::RIGHT_SHIFT_INT),
            static_cast<uint8_t>(OpCode::BITWISE_NOT_INT),

            static_cast<uint8_t>(OpCode::LOAD_LOCAL),
            static_cast<uint8_t>(OpCode::STORE_LOCAL),
            // MYT-199: type-quickened LOAD_LOCAL / STORE_LOCAL variants are
            // jittable via the generic emit path — see the switch arms in
            // JitCompiler_ControlFlow.cpp.
            static_cast<uint8_t>(OpCode::LOAD_LOCAL_INT),
            static_cast<uint8_t>(OpCode::LOAD_LOCAL_FLOAT),
            static_cast<uint8_t>(OpCode::LOAD_LOCAL_BOOL),
            static_cast<uint8_t>(OpCode::LOAD_LOCAL_BOXED_INST),
            static_cast<uint8_t>(OpCode::STORE_LOCAL_INT),
            static_cast<uint8_t>(OpCode::STORE_LOCAL_FLOAT),
            static_cast<uint8_t>(OpCode::STORE_LOCAL_BOOL),
            static_cast<uint8_t>(OpCode::STORE_LOCAL_BOXED_INST),
            static_cast<uint8_t>(OpCode::LOAD_VAR),
            static_cast<uint8_t>(OpCode::STORE_VAR),
            static_cast<uint8_t>(OpCode::LOAD_VAR_CACHED),     // MYT-204
            static_cast<uint8_t>(OpCode::STORE_VAR_CACHED),    // MYT-204

            static_cast<uint8_t>(OpCode::JUMP),
            static_cast<uint8_t>(OpCode::JUMP_IF_FALSE),
            static_cast<uint8_t>(OpCode::JUMP_IF_TRUE),
            static_cast<uint8_t>(OpCode::JUMP_IF_FALSE_OR_POP),
            static_cast<uint8_t>(OpCode::JUMP_IF_TRUE_OR_POP),
            static_cast<uint8_t>(OpCode::JUMP_BACK),
            static_cast<uint8_t>(OpCode::RETURN),
            static_cast<uint8_t>(OpCode::RETURN_VALUE),

            static_cast<uint8_t>(OpCode::CALL),
            static_cast<uint8_t>(OpCode::CALL_FAST),

            static_cast<uint8_t>(OpCode::PUSH_STRING),

            static_cast<uint8_t>(OpCode::GET_FIELD),
            static_cast<uint8_t>(OpCode::GET_FIELD_CACHED),    // MYT-194
            static_cast<uint8_t>(OpCode::SET_FIELD),
            static_cast<uint8_t>(OpCode::SET_FIELD_CACHED),    // MYT-194
            static_cast<uint8_t>(OpCode::INLINE_GET_FIELD),
            static_cast<uint8_t>(OpCode::INLINE_SET_FIELD),

            static_cast<uint8_t>(OpCode::CALL_METHOD),
            static_cast<uint8_t>(OpCode::CALL_METHOD_CACHED),  // MYT-173
            static_cast<uint8_t>(OpCode::CALL_METHOD_POLY_CACHED), // MYT-203
            static_cast<uint8_t>(OpCode::LOAD_LOCAL_CALL_CACHED),   // MYT-198
            static_cast<uint8_t>(OpCode::LOAD_LOCAL_CALL_POLY_CACHED), // MYT-203
            static_cast<uint8_t>(OpCode::LOAD_LOCAL_GET_FIELD_CACHED), // MYT-198
            static_cast<uint8_t>(OpCode::ADD_INT_CONST),       // MYT-198

            // MYT-202: compile-time fused superinstructions. Emitted natively
            // as the equivalent unfused sequence (two LOAD_LOCALs + arith,
            // etc.) by the per-category emit switches — the JIT-side speedup
            // over the interpreter comes for free once each component is
            // already JIT-emittable.
            static_cast<uint8_t>(OpCode::LOAD_LOAD_ADD_INT),
            static_cast<uint8_t>(OpCode::LOAD_LOAD_SUB_INT),
            static_cast<uint8_t>(OpCode::LOAD_LOAD_MUL_INT),
            static_cast<uint8_t>(OpCode::LOAD_GET_FIELD),
            static_cast<uint8_t>(OpCode::LOAD_STORE_LOCAL),
            static_cast<uint8_t>(OpCode::ADD_INT_STORE_LOCAL),

            static_cast<uint8_t>(OpCode::CALL_STATIC),

            static_cast<uint8_t>(OpCode::NEW_OBJECT),
            static_cast<uint8_t>(OpCode::NEW_STACK),   // MYT-134
            static_cast<uint8_t>(OpCode::NEW_ARRAY),
            static_cast<uint8_t>(OpCode::NEW_ARRAY_MULTI),
            static_cast<uint8_t>(OpCode::ARRAY_GET),
            static_cast<uint8_t>(OpCode::ARRAY_SET),
            static_cast<uint8_t>(OpCode::ARRAY_LENGTH),
            static_cast<uint8_t>(OpCode::ARRAY_GET_INT),
            static_cast<uint8_t>(OpCode::ARRAY_SET_INT),
            static_cast<uint8_t>(OpCode::ARRAY_GET_FIELD),
            static_cast<uint8_t>(OpCode::ARRAY_SET_FIELD),
            static_cast<uint8_t>(OpCode::ARRAY_GET_INT_LOCAL),
            static_cast<uint8_t>(OpCode::ARRAY_SET_INT_LOCAL),
            static_cast<uint8_t>(OpCode::ARRAY_LENGTH_LOCAL),

            static_cast<uint8_t>(OpCode::INSTANCEOF),
            static_cast<uint8_t>(OpCode::CAST),

            static_cast<uint8_t>(OpCode::NEW_VALUE_OBJECT),
            static_cast<uint8_t>(OpCode::OBJECT_TO_VALUE),

            static_cast<uint8_t>(OpCode::INVOKE_INT_ADD),
            static_cast<uint8_t>(OpCode::INVOKE_INT_SUB),
            static_cast<uint8_t>(OpCode::INVOKE_INT_MUL),
            static_cast<uint8_t>(OpCode::INVOKE_INT_DIV),
            static_cast<uint8_t>(OpCode::INVOKE_INT_MOD),
            static_cast<uint8_t>(OpCode::INVOKE_INT_NEG),
            static_cast<uint8_t>(OpCode::INVOKE_INT_ABS),
            static_cast<uint8_t>(OpCode::INVOKE_INT_EQUALS),
            static_cast<uint8_t>(OpCode::INVOKE_INT_COMPARE),
            static_cast<uint8_t>(OpCode::INVOKE_INT_GET_VALUE),
            static_cast<uint8_t>(OpCode::INVOKE_INT_LESS_THAN),
            static_cast<uint8_t>(OpCode::INVOKE_INT_LESS_EQUAL),
            static_cast<uint8_t>(OpCode::INVOKE_INT_GREATER_THAN),
            static_cast<uint8_t>(OpCode::INVOKE_INT_GREATER_EQUAL),

            static_cast<uint8_t>(OpCode::INVOKE_FLOAT_ADD),
            static_cast<uint8_t>(OpCode::INVOKE_FLOAT_SUB),
            static_cast<uint8_t>(OpCode::INVOKE_FLOAT_MUL),
            static_cast<uint8_t>(OpCode::INVOKE_FLOAT_DIV),
            static_cast<uint8_t>(OpCode::INVOKE_FLOAT_NEG),
            static_cast<uint8_t>(OpCode::INVOKE_FLOAT_ABS),
            static_cast<uint8_t>(OpCode::INVOKE_FLOAT_EQUALS),
            static_cast<uint8_t>(OpCode::INVOKE_FLOAT_COMPARE),
            static_cast<uint8_t>(OpCode::INVOKE_FLOAT_GET_VALUE),
            static_cast<uint8_t>(OpCode::INVOKE_BOOL_GET_VALUE),
            static_cast<uint8_t>(OpCode::INVOKE_FLOAT_LESS_THAN),
            static_cast<uint8_t>(OpCode::INVOKE_FLOAT_LESS_EQUAL),
            static_cast<uint8_t>(OpCode::INVOKE_FLOAT_GREATER_THAN),
            static_cast<uint8_t>(OpCode::INVOKE_FLOAT_GREATER_EQUAL),

            static_cast<uint8_t>(OpCode::GET_ITERATOR),
            static_cast<uint8_t>(OpCode::ITERATOR_HAS_NEXT),
            static_cast<uint8_t>(OpCode::ITERATOR_NEXT),
            static_cast<uint8_t>(OpCode::ITERATOR_CLOSE),

            static_cast<uint8_t>(OpCode::LINE),
            static_cast<uint8_t>(OpCode::SOURCE_FILE),
            static_cast<uint8_t>(OpCode::NOP),
            static_cast<uint8_t>(OpCode::LOOP_START),
            static_cast<uint8_t>(OpCode::LOOP_END),
            static_cast<uint8_t>(OpCode::PROFILE_ENTER),
            static_cast<uint8_t>(OpCode::PROFILE_EXIT),
            static_cast<uint8_t>(OpCode::HALT),
        };
        return supported;
    }

    static bool emitPushOps(JitEmissionState& s,
                             const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        switch (instr.opcode)
        {
            case OpCode::PUSH_INT:
            {
                if (instr.operands[0] >= s.program.getConstantPool().integers.size())
                { s.compileFailed = true; return true; }
                int64_t val = s.program.getConstantPool().getInteger(instr.operands[0]);
                Gp tmp = cc.new_gp64();
                cc.mov(tmp, val);
                cc.mov(Mem(s.stackBase, s.stackDepth * 8), tmp);
                s.slotTypes.push_back(SlotType::INT);
                s.stackDepth++;
                return true;
            }
            case OpCode::PUSH_FLOAT:
            {
                if (instr.operands[0] >= s.program.getConstantPool().floats.size())
                { s.compileFailed = true; return true; }
                double dval = s.program.getConstantPool().getFloat(instr.operands[0]);
                uint64_t bits;
                std::memcpy(&bits, &dval, sizeof(bits));
                Gp tmp = cc.new_gp64();
                cc.mov(tmp, static_cast<int64_t>(bits));
                cc.mov(Mem(s.stackBase, s.stackDepth * 8), tmp);
                s.slotTypes.push_back(SlotType::FLOAT);
                s.stackDepth++;
                return true;
            }
            case OpCode::PUSH_BOOL:
            {
                int64_t val = (instr.operands[0] != 0) ? 1 : 0;
                Gp tmp = cc.new_gp64();
                cc.mov(tmp, val);
                cc.mov(Mem(s.stackBase, s.stackDepth * 8), tmp);
                s.slotTypes.push_back(SlotType::BOOL);
                s.stackDepth++;
                return true;
            }
            case OpCode::PUSH_NULL:
                if (s.usesBoxedTypes)
                {
                    constexpr size_t vs = JitEmissionState::VALUE_SIZE;
                    Gp addr = cc.new_gp64();
                    cc.lea(addr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * vs)));
                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_box_null),
                              FuncSignature::build<void, value::Value*>());
                    inv->set_arg(0, addr);
                    s.slotTypes.push_back(SlotType::BOXED);
                }
                else
                {
                    cc.mov(qword_ptr(s.stackBase, s.stackDepth * 8), 0);
                    s.slotTypes.push_back(SlotType::INT);
                }
                s.stackDepth++;
                return true;
            default: return false;
        }
    }

    static bool emitDupOp(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        SlotType tt = topType(s);
        if (s.usesBoxedTypes && isBoxedSlotType(tt))
        {
            Gp src = cc.new_gp64();
            cc.lea(src, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
            Gp dst = cc.new_gp64();
            cc.lea(dst, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            inv->set_arg(0, dst);
            inv->set_arg(1, src);
        }
        else
        {
            Gp tmp = cc.new_gp64();
            cc.mov(tmp, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            cc.mov(Mem(s.stackBase, s.stackDepth * 8), tmp);
        }
        s.slotTypes.push_back(tt);
        s.stackDepth++;
        return true;
    }

    static bool emitSwapOp(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        SlotType t1 = s.slotTypes.size() >= 1 ? s.slotTypes[s.slotTypes.size() - 1] : SlotType::INT;
        SlotType t2 = s.slotTypes.size() >= 2 ? s.slotTypes[s.slotTypes.size() - 2] : SlotType::INT;
        bool b1 = isBoxedSlotType(t1), b2 = isBoxedSlotType(t2);
        if (s.usesBoxedTypes && (b1 || b2))
        {
            if (b1 && b2)
            {
                Gp addrA = cc.new_gp64();
                cc.lea(addrA, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
                Gp addrB = cc.new_gp64();
                cc.lea(addrB, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_swap),
                          FuncSignature::build<void, value::Value*, value::Value*>());
                inv->set_arg(0, addrA);
                inv->set_arg(1, addrB);
            }
            else
            {
                s.compileFailed = true;
                return true;
            }
        }
        else
        {
            Gp a = cc.new_gp64();
            Gp b = cc.new_gp64();
            cc.mov(a, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            cc.mov(b, Mem(s.stackBase, (s.stackDepth - 2) * 8));
            cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), b);
            cc.mov(Mem(s.stackBase, (s.stackDepth - 2) * 8), a);
        }
        if (s.slotTypes.size() >= 2)
            std::swap(s.slotTypes[s.slotTypes.size() - 1], s.slotTypes[s.slotTypes.size() - 2]);
        return true;
    }

    bool emitCoreOps(JitEmissionState& s,
                     const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        switch (instr.opcode)
        {
            case OpCode::LINE: case OpCode::SOURCE_FILE: case OpCode::NOP:
            case OpCode::LOOP_START: case OpCode::LOOP_END:
            case OpCode::PROFILE_ENTER: case OpCode::PROFILE_EXIT:
            case OpCode::HALT:
                return true;

            case OpCode::PUSH_INT: case OpCode::PUSH_FLOAT:
            case OpCode::PUSH_BOOL: case OpCode::PUSH_NULL:
                return emitPushOps(s, instr);

            case OpCode::POP:
            {
                SlotType pt = popType(s);
                s.stackDepth--;
                if (s.usesBoxedTypes && isBoxedSlotType(pt))
                {
                    Gp addr = cc.new_gp64();
                    cc.lea(addr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_destroy),
                              FuncSignature::build<void, value::Value*>());
                    inv->set_arg(0, addr);
                }
                return true;
            }

            case OpCode::DUP:  return emitDupOp(s);
            case OpCode::SWAP: return emitSwapOp(s);

            default:
                return false;
        }
    }
}
