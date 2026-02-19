#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    // ── Individual opcode emitters ─────────────────────────────────────

    static bool emitNewArray(JitEmissionState& s,
                             const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t typeIndex = static_cast<uint32_t>(instr.operands[0]);
        popType(s);

        Gp sizeVal = cc.new_gp64();
        cc.mov(sizeVal, Mem(s.stackBase, (s.stackDepth - 1) * 8));

        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(typeIndex));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_new_array),
                  FuncSignature::build<void, value::Value*, JitContext*,
                      uint32_t, int64_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);
        inv->set_arg(2, idx);
        inv->set_arg(3, sizeVal);

        s.slotTypes.push_back(SlotType::ARRAY);
        return true;
    }

    static bool emitArrayGet(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        popType(s);
        popType(s);

        Gp indexVal = cc.new_gp64();
        cc.mov(indexVal, Mem(s.stackBase, (s.stackDepth - 1) * 8));
        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_array_get),
                  FuncSignature::build<void, value::Value*, const value::Value*,
                      int64_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, arrAddr);
        inv->set_arg(2, indexVal);

        // Mirror the boxed result to stackBase so ADD_INT and other
        // primitive ops can read it from the unboxed stack.
        Gp unboxAddr = cc.new_gp64();
        cc.lea(unboxAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));
        InvokeNode* unbox;
        cc.invoke(Out(unbox), reinterpret_cast<uint64_t>(jit_unbox_int),
                  FuncSignature::build<int64_t, const value::Value*>());
        unbox->set_arg(0, unboxAddr);
        Gp unboxed = cc.new_gp64();
        unbox->set_ret(0, unboxed);
        cc.mov(Mem(s.stackBase, (s.stackDepth - 2) * 8), unboxed);

        s.stackDepth--;
        s.slotTypes.push_back(SlotType::BOXED);
        return true;
    }

    static bool emitArraySet(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        SlotType valType = popType(s);
        popType(s);
        popType(s);

        Gp valAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, valType);

        Gp indexVal = cc.new_gp64();
        cc.mov(indexVal, Mem(s.stackBase, (s.stackDepth - 2) * 8));
        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 3) * valueSize)));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_array_set),
                  FuncSignature::build<void, const value::Value*, int64_t,
                      const value::Value*>());
        inv->set_arg(0, arrAddr);
        inv->set_arg(1, indexVal);
        inv->set_arg(2, valAddr);

        if (isBoxedSlotType(valType))
            emitValueDestroy(s, s.stackDepth - 1);
        emitValueDestroy(s, s.stackDepth - 3);

        s.stackDepth -= 3;
        return true;
    }

    static bool emitArrayLength(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        popType(s);

        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_array_length),
                  FuncSignature::build<int64_t, const value::Value*>());
        inv->set_arg(0, arrAddr);
        Gp lenVal = cc.new_gp64();
        inv->set_ret(0, lenVal);

        emitValueDestroy(s, s.stackDepth - 1);

        cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), lenVal);
        s.slotTypes.push_back(SlotType::INT);
        return true;
    }

    static bool emitArrayGetInt(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        popType(s);
        popType(s);

        Gp idx = cc.new_gp64();
        cc.mov(idx, Mem(s.stackBase, (s.stackDepth - 1) * 8));
        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_array_get_int),
                  FuncSignature::build<int64_t, const value::Value*, int64_t>());
        inv->set_arg(0, arrAddr);
        inv->set_arg(1, idx);
        Gp result = cc.new_gp64();
        inv->set_ret(0, result);

        emitValueDestroy(s, s.stackDepth - 2);

        s.stackDepth -= 2;
        cc.mov(Mem(s.stackBase, s.stackDepth * 8), result);
        s.slotTypes.push_back(SlotType::INT);
        s.stackDepth++;
        return true;
    }

    static bool emitArraySetInt(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        popType(s);
        popType(s);
        popType(s);

        Gp val = cc.new_gp64();
        cc.mov(val, Mem(s.stackBase, (s.stackDepth - 1) * 8));
        Gp idx = cc.new_gp64();
        cc.mov(idx, Mem(s.stackBase, (s.stackDepth - 2) * 8));
        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 3) * valueSize)));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_array_set_int),
                  FuncSignature::build<void, const value::Value*, int64_t, int64_t>());
        inv->set_arg(0, arrAddr);
        inv->set_arg(1, idx);
        inv->set_arg(2, val);

        emitValueDestroy(s, s.stackDepth - 3);

        s.stackDepth -= 3;
        return true;
    }

    static bool emitArrayGetField(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t fieldNameIndex = static_cast<uint32_t>(instr.operands[0]);
        popType(s);
        popType(s);

        Gp idx = cc.new_gp64();
        cc.mov(idx, Mem(s.stackBase, (s.stackDepth - 1) * 8));
        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));
        Gp pPtr = cc.new_gp64();
        cc.mov(pPtr, s.progPtr);
        Gp fnIdx = cc.new_gp64();
        cc.mov(fnIdx, static_cast<int64_t>(fieldNameIndex));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_array_get_field),
                  FuncSignature::build<void, value::Value*, const value::Value*,
                      int64_t, const vm::bytecode::BytecodeProgram*, uint32_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, arrAddr);
        inv->set_arg(2, idx);
        inv->set_arg(3, pPtr);
        inv->set_arg(4, fnIdx);

        s.stackDepth -= 2;
        s.slotTypes.push_back(SlotType::BOXED);
        s.stackDepth++;
        return true;
    }

    static bool emitArraySetField(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t fieldNameIndex = static_cast<uint32_t>(instr.operands[0]);
        SlotType valType = popType(s);
        popType(s);
        popType(s);

        Gp newValAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, valType);

        Gp idx = cc.new_gp64();
        cc.mov(idx, Mem(s.stackBase, (s.stackDepth - 2) * 8));
        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 3) * valueSize)));
        Gp pPtr = cc.new_gp64();
        cc.mov(pPtr, s.progPtr);
        Gp fnIdx = cc.new_gp64();
        cc.mov(fnIdx, static_cast<int64_t>(fieldNameIndex));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_array_set_field),
                  FuncSignature::build<void, const value::Value*, int64_t,
                      const value::Value*,
                      const vm::bytecode::BytecodeProgram*, uint32_t>());
        inv->set_arg(0, arrAddr);
        inv->set_arg(1, idx);
        inv->set_arg(2, newValAddr);
        inv->set_arg(3, pPtr);
        inv->set_arg(4, fnIdx);

        if (isBoxedSlotType(valType))
            emitValueDestroy(s, s.stackDepth - 1);
        emitValueDestroy(s, s.stackDepth - 3);

        s.stackDepth -= 3;
        return true;
    }

    // ── Group dispatchers ───────────────────────────────────────────────

    static bool emitBasicArrayOps(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr)
    {
        switch (instr.opcode)
        {
            case OpCode::NEW_ARRAY:     return emitNewArray(s, instr);
            case OpCode::ARRAY_GET:     return emitArrayGet(s);
            case OpCode::ARRAY_SET:     return emitArraySet(s);
            case OpCode::ARRAY_LENGTH:  return emitArrayLength(s);
            default:                    return false;
        }
    }

    static bool emitTypedArrayOps(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr)
    {
        switch (instr.opcode)
        {
            case OpCode::ARRAY_GET_INT:   return emitArrayGetInt(s);
            case OpCode::ARRAY_SET_INT:   return emitArraySetInt(s);
            case OpCode::ARRAY_GET_FIELD: return emitArrayGetField(s, instr);
            case OpCode::ARRAY_SET_FIELD: return emitArraySetField(s, instr);
            default:                      return false;
        }
    }

    // ── Public entry point ──────────────────────────────────────────────

    bool emitArrayOps(JitEmissionState& s,
                      const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (emitBasicArrayOps(s, instr)) return true;
        if (emitTypedArrayOps(s, instr)) return true;
        return false;
    }
}
