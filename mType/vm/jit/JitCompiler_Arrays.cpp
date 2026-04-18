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

    // MYT-146: NEW_ARRAY_MULTI. Operands: [typeIdx, totalDims, specifiedDims].
    // Dimension sizes are on the operand stack (specifiedDims of them), with
    // dim_0 deepest and dim_last on top. Box each into ctx->callArgs[0..N-1],
    // call helper, push the resulting boxed array as the first dimension-slot's
    // replacement.
    static bool emitNewArrayMulti(JitEmissionState& s,
                                   const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t typeIndex      = static_cast<uint32_t>(instr.operands[0]);
        uint32_t totalDims      = static_cast<uint32_t>(instr.operands[1]);
        uint32_t specifiedDims  = instr.operands.size() > 2
                                   ? static_cast<uint32_t>(instr.operands[2])
                                   : totalDims;

        // Box each dimension size from the JIT stack into ctx->callArgs[i].
        // emitBoxCallArgs takes the top `specifiedDims` slots in order and
        // preserves dim_0-deepest → callArgs[0] semantics.
        emitBoxCallArgs(s, specifiedDims);
        // Pop the size slots (all primitive INT — emitPopAndDestroyArgs is a
        // no-op for unboxed slots apart from decrementing stackDepth).
        emitPopAndDestroyArgs(s, specifiedDims);

        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        Gp typeIdxReg = cc.new_gp64();
        cc.mov(typeIdxReg, static_cast<int64_t>(typeIndex));
        Gp totalDimsReg = cc.new_gp64();
        cc.mov(totalDimsReg, static_cast<int64_t>(totalDims));
        Gp specifiedDimsReg = cc.new_gp64();
        cc.mov(specifiedDimsReg, static_cast<int64_t>(specifiedDims));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_new_array_multi),
                  FuncSignature::build<void, value::Value*, JitContext*,
                      uint32_t, uint32_t, uint32_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);
        inv->set_arg(2, typeIdxReg);
        inv->set_arg(3, totalDimsReg);
        inv->set_arg(4, specifiedDimsReg);

        s.slotTypes.push_back(SlotType::ARRAY);
        s.stackDepth++;
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

    // Extract array info (dataPtr + length) for a given boxed stack slot.
    // Uses the cache if available, otherwise calls jit_array_extract_info once.
    static void emitExtractArrayInfo(JitEmissionState& s, int arrSlot,
                                      Gp arrAddr, Gp& dataPtr, Gp& arrLen)
    {
        auto& cc = s.cc;
        auto cacheIt = s.arrayInfoCache.find(arrSlot);
        if (cacheIt != s.arrayInfoCache.end())
        {
            dataPtr = cacheIt->second.dataPtr;
            arrLen = cacheIt->second.length;
            return;
        }

        Mem infoMem = cc.new_stack(16, 8);
        Gp infoAddr = cc.new_gp64();
        cc.lea(infoAddr, infoMem);

        InvokeNode* extractInv;
        cc.invoke(Out(extractInv), reinterpret_cast<uint64_t>(jit_array_extract_info),
                  FuncSignature::build<void, const value::Value*, JitArrayInfo*>());
        extractInv->set_arg(0, arrAddr);
        extractInv->set_arg(1, infoAddr);

        dataPtr = cc.new_gp64();
        cc.mov(dataPtr, Mem(infoAddr, 0));   // JitArrayInfo::data
        arrLen = cc.new_gp64();
        cc.mov(arrLen, Mem(infoAddr, 8));    // JitArrayInfo::length

        s.arrayInfoCache[arrSlot] = {dataPtr, arrLen};
    }

    static bool emitArrayLength(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        popType(s);

        int arrSlot = s.stackDepth - 1;
        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.boxedBase, static_cast<int32_t>(arrSlot * valueSize)));

        // Use emitExtractArrayInfo to share cache with ARRAY_GET_INT/ARRAY_SET_INT
        Gp dataPtr, arrLen;
        emitExtractArrayInfo(s, arrSlot, arrAddr, dataPtr, arrLen);

        emitValueDestroy(s, s.stackDepth - 1);

        cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), arrLen);
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
        int arrSlot = s.stackDepth - 2;
        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.boxedBase, static_cast<int32_t>(arrSlot * valueSize)));

        // Extract array info (cached or single combined call)
        Gp dataPtr, arrLen;
        emitExtractArrayInfo(s, arrSlot, arrAddr, dataPtr, arrLen);

        // Bounds check: unsigned compare catches negative + overflow
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

        // Extract array info (cached or single combined call)
        Gp dataPtr, arrLen;
        emitExtractArrayInfo(s, arrSlot, arrAddr, dataPtr, arrLen);

        // Bounds check: unsigned compare catches negative + overflow
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

        // Slow path: heterogeneous array fallback
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

    // ── Fused local-array emitters (no copy/destroy overhead) ──────────

    static bool emitArrayLengthLocal(JitEmissionState& s,
                                     const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        size_t localSlot = instr.operands[0];

        // Read array directly from locals — no jit_value_copy
        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.localsBase, static_cast<int32_t>(localSlot * s.localStride)));

        Gp dataPtr, arrLen;
        emitExtractArrayInfo(s, static_cast<int>(localSlot + 10000), arrAddr, dataPtr, arrLen);

        // No emitValueDestroy — array stays in locals
        cc.mov(Mem(s.stackBase, s.stackDepth * 8), arrLen);
        s.slotTypes.push_back(SlotType::INT);
        s.stackDepth++;
        return true;
    }

    static bool emitArrayGetIntLocal(JitEmissionState& s,
                                     const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        size_t localSlot = instr.operands[0];
        popType(s); // pop index type

        Gp idx = cc.new_gp64();
        cc.mov(idx, Mem(s.stackBase, (s.stackDepth - 1) * 8));

        // Read array directly from locals — no jit_value_copy
        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.localsBase, static_cast<int32_t>(localSlot * s.localStride)));

        Gp dataPtr, arrLen;
        emitExtractArrayInfo(s, static_cast<int>(localSlot + 10000), arrAddr, dataPtr, arrLen);

        // Bounds check
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

        // Fast path: direct memory read
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

        // No emitValueDestroy — array stays in locals
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
        size_t localSlot = instr.operands[0];
        popType(s); // pop value type
        popType(s); // pop index type

        Gp val = cc.new_gp64();
        cc.mov(val, Mem(s.stackBase, (s.stackDepth - 1) * 8));
        Gp idx = cc.new_gp64();
        cc.mov(idx, Mem(s.stackBase, (s.stackDepth - 2) * 8));

        // Read array directly from locals — no jit_value_copy
        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.localsBase, static_cast<int32_t>(localSlot * s.localStride)));

        Gp dataPtr, arrLen;
        emitExtractArrayInfo(s, static_cast<int>(localSlot + 10000), arrAddr, dataPtr, arrLen);

        // Bounds check
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

        // Fast path: direct memory write
        Gp addr = cc.new_gp64();
        cc.mov(addr, idx);
        cc.shl(addr, 3);
        cc.add(addr, dataPtr);
        cc.mov(Mem(addr, 0), val);
        cc.jmp(done);

        // Slow path: heterogeneous array fallback
        cc.bind(slowPath);
        InvokeNode* slowInv;
        cc.invoke(Out(slowInv), reinterpret_cast<uint64_t>(jit_array_set_int),
                  FuncSignature::build<void, const value::Value*, int64_t, int64_t>());
        slowInv->set_arg(0, arrAddr);
        slowInv->set_arg(1, idx);
        slowInv->set_arg(2, val);

        cc.bind(done);

        // No emitValueDestroy — array stays in locals
        s.stackDepth -= 2;
        return true;
    }

    // ── Group dispatchers ───────────────────────────────────────────────

    static bool emitBasicArrayOps(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr)
    {
        switch (instr.opcode)
        {
            case OpCode::NEW_ARRAY:         return emitNewArray(s, instr);
            case OpCode::NEW_ARRAY_MULTI:   return emitNewArrayMulti(s, instr);
            case OpCode::ARRAY_GET:         return emitArrayGet(s);
            case OpCode::ARRAY_SET:         return emitArraySet(s);
            case OpCode::ARRAY_LENGTH:      return emitArrayLength(s);
            default:                        return false;
        }
    }

    static bool emitTypedArrayOps(JitEmissionState& s,
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

    // ── Public entry point ──────────────────────────────────────────────

    bool emitArrayOps(JitEmissionState& s,
                      const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (emitBasicArrayOps(s, instr)) return true;
        if (emitTypedArrayOps(s, instr)) return true;
        return false;
    }
}
