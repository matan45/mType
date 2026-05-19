#include "JitCompiler_Arrays_Internal.hpp"
#include <asmjit/x86.h>
#include <cstddef>
#include <cstdint>
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"

namespace vm::jit::detail
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    static bool emitArrayGetInt(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        popType(s);
        popType(s);

        Gp idx = cc.new_gp64();
        cc.mov(idx, Mem(s.stackBase, (s.stackDepth - 1) * 8));
        int arrSlot = s.stackDepth - 2;
        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.boxedBase, static_cast<int32_t>(arrSlot * valueSize)));

        // Operand-stack slots are reused for different arrays inside hot loops,
        // so do not cache by stack position here.
        Gp dataPtr, arrLen;
        emitExtractArrayInfo(s, -1, arrAddr, dataPtr, arrLen);

        // Bounds check — unsigned compare catches negative + overflow.
        Label boundsOk = cc.new_label();
        cc.cmp(idx, arrLen);
        cc.jb(boundsOk);

        InvokeNode* oobInv;
        cc.invoke(Out(oobInv), reinterpret_cast<uint64_t>(jit_throw_array_oob),
                  FuncSignature::build<void, int64_t, int64_t>());
        oobInv->set_arg(0, idx);
        oobInv->set_arg(1, arrLen);

        cc.bind(boundsOk);

        Gp result = cc.new_gp64();
        Label slowPath = cc.new_label();
        Label done = cc.new_label();

        cc.test(dataPtr, dataPtr);
        cc.jz(slowPath);

        // Fast path: direct memory read — dataPtr[idx * 8]
        Gp addr = cc.new_gp64();
        cc.mov(addr, idx);
        cc.shl(addr, 3);
        cc.add(addr, dataPtr);
        cc.mov(result, Mem(addr, 0));
        cc.jmp(done);

        // Slow path: heterogeneous array fallback
        cc.bind(slowPath);
        InvokeNode* slowInv;
        cc.invoke(Out(slowInv), reinterpret_cast<uint64_t>(jit_array_get_int),
                  FuncSignature::build<int64_t, const value::Value*, int64_t>());
        slowInv->set_arg(0, arrAddr);
        slowInv->set_arg(1, idx);
        slowInv->set_ret(0, result);

        cc.bind(done);

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
        int arrSlot = s.stackDepth - 3;
        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.boxedBase, static_cast<int32_t>(arrSlot * valueSize)));

        Gp dataPtr, arrLen;
        emitExtractArrayInfo(s, -1, arrAddr, dataPtr, arrLen);

        Label boundsOk = cc.new_label();
        cc.cmp(idx, arrLen);
        cc.jb(boundsOk);

        InvokeNode* oobInv;
        cc.invoke(Out(oobInv), reinterpret_cast<uint64_t>(jit_throw_array_oob),
                  FuncSignature::build<void, int64_t, int64_t>());
        oobInv->set_arg(0, idx);
        oobInv->set_arg(1, arrLen);

        cc.bind(boundsOk);

        Label slowPath = cc.new_label();
        Label done = cc.new_label();

        cc.test(dataPtr, dataPtr);
        cc.jz(slowPath);

        // Fast path: direct memory write — dataPtr[idx * 8] = val
        Gp addr = cc.new_gp64();
        cc.mov(addr, idx);
        cc.shl(addr, 3);
        cc.add(addr, dataPtr);
        cc.mov(Mem(addr, 0), val);
        cc.jmp(done);

        cc.bind(slowPath);
        InvokeNode* slowInv;
        cc.invoke(Out(slowInv), reinterpret_cast<uint64_t>(jit_array_set_int),
                  FuncSignature::build<void, const value::Value*, int64_t, int64_t>());
        slowInv->set_arg(0, arrAddr);
        slowInv->set_arg(1, idx);
        slowInv->set_arg(2, val);

        cc.bind(done);

        emitValueDestroy(s, s.stackDepth - 3);

        s.stackDepth -= 3;
        return true;
    }

    static bool emitArrayGetField(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t fieldNameIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
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

        Gp unboxAddr = cc.new_gp64();
        cc.lea(unboxAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));
        InvokeNode* unbox;
        cc.invoke(Out(unbox), reinterpret_cast<uint64_t>(jit_unbox_int),
                  FuncSignature::build<int64_t, const value::Value*>());
        unbox->set_arg(0, unboxAddr);
        Gp unboxed = cc.new_gp64();
        unbox->set_ret(0, unboxed);
        cc.mov(Mem(s.stackBase, (s.stackDepth - 2) * 8), unboxed);

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
        uint32_t fieldNameIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
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

    // Fused local-array emitters (no copy/destroy overhead).

    static bool emitArrayLengthLocal(JitEmissionState& s,
                                     const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        size_t localSlot = instr.inlineOperands[0];

        // Read array directly from locals — no jit_value_copy.
        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.localsBase, static_cast<int32_t>(localSlot * s.localStride)));

        Gp dataPtr, arrLen;
        emitExtractArrayInfo(s, static_cast<int>(localSlot + 10000), arrAddr, dataPtr, arrLen);

        // No emitValueDestroy — array stays in locals.
        cc.mov(Mem(s.stackBase, s.stackDepth * 8), arrLen);
        s.slotTypes.push_back(SlotType::INT);
        s.stackDepth++;
        return true;
    }

    static bool emitArrayGetIntLocal(JitEmissionState& s,
                                     const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        size_t localSlot = instr.inlineOperands[0];
        popType(s);

        Gp idx = cc.new_gp64();
        cc.mov(idx, Mem(s.stackBase, (s.stackDepth - 1) * 8));

        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.localsBase, static_cast<int32_t>(localSlot * s.localStride)));

        Gp dataPtr, arrLen;
        emitExtractArrayInfo(s, static_cast<int>(localSlot + 10000), arrAddr, dataPtr, arrLen);

        Label boundsOk = cc.new_label();
        cc.cmp(idx, arrLen);
        cc.jb(boundsOk);

        InvokeNode* oobInv;
        cc.invoke(Out(oobInv), reinterpret_cast<uint64_t>(jit_throw_array_oob),
                  FuncSignature::build<void, int64_t, int64_t>());
        oobInv->set_arg(0, idx);
        oobInv->set_arg(1, arrLen);

        cc.bind(boundsOk);

        Gp result = cc.new_gp64();
        Label slowPath = cc.new_label();
        Label done = cc.new_label();

        cc.test(dataPtr, dataPtr);
        cc.jz(slowPath);

        Gp addr = cc.new_gp64();
        cc.mov(addr, idx);
        cc.shl(addr, 3);
        cc.add(addr, dataPtr);
        cc.mov(result, Mem(addr, 0));
        cc.jmp(done);

        cc.bind(slowPath);
        InvokeNode* slowInv;
        cc.invoke(Out(slowInv), reinterpret_cast<uint64_t>(jit_array_get_int),
                  FuncSignature::build<int64_t, const value::Value*, int64_t>());
        slowInv->set_arg(0, arrAddr);
        slowInv->set_arg(1, idx);
        slowInv->set_ret(0, result);

        cc.bind(done);

        // No emitValueDestroy — array stays in locals.
        s.stackDepth--;
        cc.mov(Mem(s.stackBase, s.stackDepth * 8), result);
        s.slotTypes.push_back(SlotType::INT);
        s.stackDepth++;
        return true;
    }

    static bool emitArraySetIntLocal(JitEmissionState& s,
                                     const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        size_t localSlot = instr.inlineOperands[0];
        popType(s);
        popType(s);

        Gp val = cc.new_gp64();
        cc.mov(val, Mem(s.stackBase, (s.stackDepth - 1) * 8));
        Gp idx = cc.new_gp64();
        cc.mov(idx, Mem(s.stackBase, (s.stackDepth - 2) * 8));

        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.localsBase, static_cast<int32_t>(localSlot * s.localStride)));

        Gp dataPtr, arrLen;
        emitExtractArrayInfo(s, static_cast<int>(localSlot + 10000), arrAddr, dataPtr, arrLen);

        Label boundsOk = cc.new_label();
        cc.cmp(idx, arrLen);
        cc.jb(boundsOk);

        InvokeNode* oobInv;
        cc.invoke(Out(oobInv), reinterpret_cast<uint64_t>(jit_throw_array_oob),
                  FuncSignature::build<void, int64_t, int64_t>());
        oobInv->set_arg(0, idx);
        oobInv->set_arg(1, arrLen);

        cc.bind(boundsOk);

        Label slowPath = cc.new_label();
        Label done = cc.new_label();

        cc.test(dataPtr, dataPtr);
        cc.jz(slowPath);

        Gp addr = cc.new_gp64();
        cc.mov(addr, idx);
        cc.shl(addr, 3);
        cc.add(addr, dataPtr);
        cc.mov(Mem(addr, 0), val);
        cc.jmp(done);

        cc.bind(slowPath);
        InvokeNode* slowInv;
        cc.invoke(Out(slowInv), reinterpret_cast<uint64_t>(jit_array_set_int),
                  FuncSignature::build<void, const value::Value*, int64_t, int64_t>());
        slowInv->set_arg(0, arrAddr);
        slowInv->set_arg(1, idx);
        slowInv->set_arg(2, val);

        cc.bind(done);

        // No emitValueDestroy — array stays in locals.
        s.stackDepth -= 2;
        return true;
    }

    bool emitTypedArrayOps(JitEmissionState& s,
                           const bytecode::BytecodeProgram::Instruction& instr)
    {
        switch (instr.opcode)
        {
            case OpCode::ARRAY_GET_INT:       return emitArrayGetInt(s);
            case OpCode::ARRAY_SET_INT:       return emitArraySetInt(s);
            case OpCode::ARRAY_GET_FIELD:     return emitArrayGetField(s, instr);
            case OpCode::ARRAY_SET_FIELD:     return emitArraySetField(s, instr);
            case OpCode::ARRAY_GET_INT_LOCAL: return emitArrayGetIntLocal(s, instr);
            case OpCode::ARRAY_SET_INT_LOCAL: return emitArraySetIntLocal(s, instr);
            case OpCode::ARRAY_LENGTH_LOCAL:  return emitArrayLengthLocal(s, instr);
            default:                          return false;
        }
    }
}
