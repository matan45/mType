#include "JitEmissionState.hpp"
#include "JitCompiler_Arrays_Internal.hpp"
#include <asmjit/x86.h>
#include <cstddef>
#include <cstdint>
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    static bool emitNewArray(JitEmissionState& s,
                             const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t typeIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
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
        uint32_t typeIndex      = static_cast<uint32_t>(instr.inlineOperands[0]);
        uint32_t totalDims      = static_cast<uint32_t>(instr.inlineOperands[1]);
        uint32_t specifiedDims  = instr.numOperands() > 2
                                   ? static_cast<uint32_t>(instr.inlineOperands[2])
                                   : totalDims;

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

    static bool emitArrayGet(JitEmissionState& s, bool alias = false)
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

        // MYT-303: ARRAY_GET_ALIAS routes to jit_array_get_alias so SoA-object
        // arrays demote before the result is captured by a local; the cheap
        // non-SoA case is a single null-check inside the helper.
        const auto helper = alias ? reinterpret_cast<uint64_t>(jit_array_get_alias)
                                  : reinterpret_cast<uint64_t>(jit_array_get);
        InvokeNode* inv;
        cc.invoke(Out(inv), helper,
                  FuncSignature::build<void, value::Value*, const value::Value*,
                      int64_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, arrAddr);
        inv->set_arg(2, indexVal);

        // Mirror the boxed result to stackBase so ADD_INT and other primitive
        // ops can read it from the unboxed stack.
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

        int arrSlot = s.stackDepth - 1;
        Gp arrAddr = cc.new_gp64();
        cc.lea(arrAddr, Mem(s.boxedBase, static_cast<int32_t>(arrSlot * valueSize)));

        // Operand-stack slots are reused for different arrays inside hot loops,
        // so do not cache by stack position here.
        Gp dataPtr, arrLen;
        detail::emitExtractArrayInfo(s, -1, arrAddr, dataPtr, arrLen);

        emitValueDestroy(s, s.stackDepth - 1);

        cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), arrLen);
        s.slotTypes.push_back(SlotType::INT);
        return true;
    }

    static bool emitBasicArrayOps(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr)
    {
        switch (instr.opcode)
        {
            case OpCode::NEW_ARRAY:         return emitNewArray(s, instr);
            case OpCode::NEW_ARRAY_MULTI:   return emitNewArrayMulti(s, instr);
            case OpCode::ARRAY_GET:         return emitArrayGet(s);
            case OpCode::ARRAY_GET_ALIAS:   return emitArrayGet(s, /*alias=*/true);
            case OpCode::ARRAY_SET:         return emitArraySet(s);
            case OpCode::ARRAY_LENGTH:      return emitArrayLength(s);
            default:                        return false;
        }
    }

    bool emitArrayOps(JitEmissionState& s,
                      const bytecode::BytecodeProgram::Instruction& instr)
    {
        // MYT-211: array emitters use cc.invoke for bounds-throw and
        // jit_value_copy paths, and read stackBase via Mem(...). They are
        // not hint-aware, so flush before any of them runs. No-op when the
        // cache is empty (boxed mode) or when the previous emit already
        // flushed (e.g. label bind, helper invoke).
        flushAllHints(s);
        if (emitBasicArrayOps(s, instr)) return true;
        if (detail::emitTypedArrayOps(s, instr)) return true;
        return false;
    }
}
