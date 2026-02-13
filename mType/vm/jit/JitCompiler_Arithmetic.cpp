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

            static_cast<uint8_t>(OpCode::LOAD_LOCAL),
            static_cast<uint8_t>(OpCode::STORE_LOCAL),

            static_cast<uint8_t>(OpCode::JUMP),
            static_cast<uint8_t>(OpCode::JUMP_IF_FALSE),
            static_cast<uint8_t>(OpCode::JUMP_IF_TRUE),
            static_cast<uint8_t>(OpCode::JUMP_BACK),
            static_cast<uint8_t>(OpCode::RETURN),
            static_cast<uint8_t>(OpCode::RETURN_VALUE),

            static_cast<uint8_t>(OpCode::CALL),

            static_cast<uint8_t>(OpCode::PUSH_STRING),

            static_cast<uint8_t>(OpCode::GET_FIELD),
            static_cast<uint8_t>(OpCode::SET_FIELD),

            static_cast<uint8_t>(OpCode::CALL_METHOD),
            static_cast<uint8_t>(OpCode::CALL_STATIC),

            static_cast<uint8_t>(OpCode::NEW_OBJECT),
            static_cast<uint8_t>(OpCode::NEW_ARRAY),
            static_cast<uint8_t>(OpCode::ARRAY_GET),
            static_cast<uint8_t>(OpCode::ARRAY_SET),
            static_cast<uint8_t>(OpCode::ARRAY_LENGTH),

            static_cast<uint8_t>(OpCode::INSTANCEOF),
            static_cast<uint8_t>(OpCode::CAST),

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

    // ===== Core ops: no-ops, stack ops =====

    bool emitCoreOps(JitEmissionState& s,
                     const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        switch (instr.opcode)
        {
            case OpCode::LINE:
            case OpCode::SOURCE_FILE:
            case OpCode::NOP:
            case OpCode::LOOP_START:
            case OpCode::LOOP_END:
            case OpCode::PROFILE_ENTER:
            case OpCode::PROFILE_EXIT:
            case OpCode::HALT:
                return true;

            case OpCode::PUSH_INT:
            {
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
                double dval = s.program.getConstantPool().getFloat(instr.operands[0]);
                float fval = static_cast<float>(dval);
                uint32_t bits;
                std::memcpy(&bits, &fval, sizeof(bits));
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
            {
                cc.mov(Mem(s.stackBase, s.stackDepth * 8), 0);
                s.slotTypes.push_back(SlotType::INT);
                s.stackDepth++;
                return true;
            }

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

            case OpCode::DUP:
            {
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

            case OpCode::SWAP:
            {
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

            default:
                return false;
        }
    }

    // ===== Arithmetic ops =====

    bool emitArithmeticOps(JitEmissionState& s,
                           const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;

        switch (instr.opcode)
        {
            // --- Integer arithmetic ---
            case OpCode::ADD_INT:
            {
                s.stackDepth--;
                popType(s); popType(s);
                Gp right = cc.new_gp64();
                cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
                cc.add(Mem(s.stackBase, (s.stackDepth - 1) * 8), right);
                s.slotTypes.push_back(SlotType::INT);
                return true;
            }

            case OpCode::SUB_INT:
            {
                s.stackDepth--;
                popType(s); popType(s);
                Gp right = cc.new_gp64();
                cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
                cc.sub(Mem(s.stackBase, (s.stackDepth - 1) * 8), right);
                s.slotTypes.push_back(SlotType::INT);
                return true;
            }

            case OpCode::MUL_INT:
            {
                s.stackDepth--;
                popType(s); popType(s);
                Gp left = cc.new_gp64();
                Gp right = cc.new_gp64();
                cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
                cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                cc.imul(left, right);
                cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), left);
                s.slotTypes.push_back(SlotType::INT);
                return true;
            }

            case OpCode::DIV_INT:
            {
                s.stackDepth--;
                popType(s); popType(s);
                Gp right = cc.new_gp64();
                cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
                cc.test(right, right);
                Label notZero = cc.new_label();
                cc.jnz(notZero);
                InvokeNode* invDZ;
                cc.invoke(Out(invDZ), reinterpret_cast<uint64_t>(jit_throw_div_by_zero),
                          FuncSignature::build<void>());
                cc.bind(notZero);
                Gp left = cc.new_gp64();
                cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                Gp raxReg = cc.new_gp64();
                Gp rdxReg = cc.new_gp64();
                cc.mov(raxReg, left);
                cc.cqo(rdxReg, raxReg);
                cc.idiv(rdxReg, raxReg, right);
                cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), raxReg);
                s.slotTypes.push_back(SlotType::INT);
                return true;
            }

            case OpCode::NEG:
                cc.neg(Mem(s.stackBase, (s.stackDepth - 1) * 8));
                return true;

            case OpCode::INC:
            {
                Gp tmp = cc.new_gp64();
                cc.mov(tmp, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                cc.inc(tmp);
                cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), tmp);
                return true;
            }

            case OpCode::DEC:
            {
                Gp tmp = cc.new_gp64();
                cc.mov(tmp, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                cc.dec(tmp);
                cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), tmp);
                return true;
            }

            // --- Float arithmetic ---
            case OpCode::ADD_FLOAT: case OpCode::SUB_FLOAT:
            case OpCode::MUL_FLOAT: case OpCode::DIV_FLOAT:
            {
                s.stackDepth--;
                popType(s); popType(s);
                Vec right = cc.new_xmm();
                Vec left = cc.new_xmm();
                cc.movss(right, Mem(s.stackBase, s.stackDepth * 8));
                cc.movss(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                if (instr.opcode == OpCode::ADD_FLOAT) cc.addss(left, right);
                else if (instr.opcode == OpCode::SUB_FLOAT) cc.subss(left, right);
                else if (instr.opcode == OpCode::MUL_FLOAT) cc.mulss(left, right);
                else cc.divss(left, right);
                cc.movss(Mem(s.stackBase, (s.stackDepth - 1) * 8), left);
                s.slotTypes.push_back(SlotType::FLOAT);
                return true;
            }

            // --- Generic arithmetic ---
            case OpCode::ADD: case OpCode::SUB: case OpCode::MUL:
            {
                SlotType rType = popType(s);
                SlotType lType = popType(s);
                s.stackDepth--;

                if (lType == SlotType::INT && rType == SlotType::INT)
                {
                    if (instr.opcode == OpCode::MUL)
                    {
                        Gp left = cc.new_gp64();
                        Gp right = cc.new_gp64();
                        cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
                        cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                        cc.imul(left, right);
                        cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), left);
                    }
                    else
                    {
                        Gp right = cc.new_gp64();
                        cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
                        if (instr.opcode == OpCode::ADD)
                            cc.add(Mem(s.stackBase, (s.stackDepth - 1) * 8), right);
                        else
                            cc.sub(Mem(s.stackBase, (s.stackDepth - 1) * 8), right);
                    }
                    s.slotTypes.push_back(SlotType::INT);
                }
                else if (lType == SlotType::FLOAT && rType == SlotType::FLOAT)
                {
                    Vec right = cc.new_xmm();
                    Vec left = cc.new_xmm();
                    cc.movss(right, Mem(s.stackBase, s.stackDepth * 8));
                    cc.movss(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                    if (instr.opcode == OpCode::ADD) cc.addss(left, right);
                    else if (instr.opcode == OpCode::SUB) cc.subss(left, right);
                    else cc.mulss(left, right);
                    cc.movss(Mem(s.stackBase, (s.stackDepth - 1) * 8), left);
                    s.slotTypes.push_back(SlotType::FLOAT);
                }
                else
                {
                    uint64_t fn;
                    if (instr.opcode == OpCode::ADD) fn = (uint64_t)jit_generic_add;
                    else if (instr.opcode == OpCode::SUB) fn = (uint64_t)jit_generic_sub;
                    else fn = (uint64_t)jit_generic_mul;
                    emitGenericBinop(s, fn, lType, rType);
                }
                return true;
            }

            case OpCode::DIV: case OpCode::MOD:
            {
                SlotType rType = popType(s);
                SlotType lType = popType(s);
                s.stackDepth--;

                if (lType == SlotType::INT && rType == SlotType::INT)
                {
                    Gp right = cc.new_gp64();
                    cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
                    cc.test(right, right);
                    Label nz = cc.new_label();
                    cc.jnz(nz);
                    InvokeNode* dz;
                    cc.invoke(Out(dz), (uint64_t)jit_throw_div_by_zero,
                              FuncSignature::build<void>());
                    cc.bind(nz);
                    Gp left = cc.new_gp64();
                    cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                    Gp rax = cc.new_gp64();
                    Gp rdx = cc.new_gp64();
                    cc.mov(rax, left);
                    cc.cqo(rdx, rax);
                    cc.idiv(rdx, rax, right);
                    cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8),
                           instr.opcode == OpCode::DIV ? rax : rdx);
                    s.slotTypes.push_back(SlotType::INT);
                }
                else
                {
                    uint64_t fn = (instr.opcode == OpCode::DIV)
                        ? (uint64_t)jit_generic_div : (uint64_t)jit_generic_mod;
                    emitGenericBinop(s, fn, lType, rType);
                }
                return true;
            }

            // --- Comparisons ---
            case OpCode::EQ:  case OpCode::EQ_INT: emitCmp(s, CmpOp::EQ); return true;
            case OpCode::NE:  case OpCode::NE_INT: emitCmp(s, CmpOp::NE); return true;
            case OpCode::LT:  case OpCode::LT_INT: emitCmp(s, CmpOp::LT); return true;
            case OpCode::GT:  case OpCode::GT_INT: emitCmp(s, CmpOp::GT); return true;
            case OpCode::LE:                        emitCmp(s, CmpOp::LE); return true;
            case OpCode::GE:                        emitCmp(s, CmpOp::GE); return true;

            // --- Logical ---
            case OpCode::NOT:
            {
                Gp val = cc.new_gp64();
                Gp result = cc.new_gp64();
                cc.mov(val, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                cc.xor_(result, result);
                cc.test(val, val);
                cc.sete(result.r8());
                cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), result);
                if (!s.slotTypes.empty()) s.slotTypes.back() = SlotType::BOOL;
                return true;
            }

            case OpCode::AND:
            {
                s.stackDepth--;
                popType(s); popType(s);
                Gp right = cc.new_gp64();
                Gp left = cc.new_gp64();
                cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
                cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                Gp r1 = cc.new_gp64();
                Gp r2 = cc.new_gp64();
                cc.xor_(r1, r1);
                cc.test(left, left);
                cc.setne(r1.r8());
                cc.xor_(r2, r2);
                cc.test(right, right);
                cc.setne(r2.r8());
                cc.and_(r1, r2);
                cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), r1);
                s.slotTypes.push_back(SlotType::BOOL);
                return true;
            }

            case OpCode::OR:
            {
                s.stackDepth--;
                popType(s); popType(s);
                Gp right = cc.new_gp64();
                Gp left = cc.new_gp64();
                cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
                cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                Gp r1 = cc.new_gp64();
                Gp r2 = cc.new_gp64();
                cc.xor_(r1, r1);
                cc.test(left, left);
                cc.setne(r1.r8());
                cc.xor_(r2, r2);
                cc.test(right, right);
                cc.setne(r2.r8());
                cc.or_(r1, r2);
                cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), r1);
                s.slotTypes.push_back(SlotType::BOOL);
                return true;
            }

            default:
                return false;
        }
    }
}
