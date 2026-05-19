#include "JitEmissionState.hpp"
#include "JitCompiler_Objects.hpp"
#include "JitHelpers.hpp"
#include <asmjit/x86.h>
#include <cstddef>
#include <cstdint>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;

    bool emitInstanceofOp(JitEmissionState& s,
                          const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        uint32_t typeIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
        SlotType vType = popType(s);
        Gp valAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, vType);
        Gp pPtr = cc.new_gp64();
        cc.mov(pPtr, s.progPtr);
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(typeIndex));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_instanceof),
                  FuncSignature::build<int64_t, const value::Value*,
                      const vm::bytecode::BytecodeProgram*, uint32_t>());
        inv->set_arg(0, valAddr);
        inv->set_arg(1, pPtr);
        inv->set_arg(2, idx);
        Gp result = cc.new_gp64();
        inv->set_ret(0, result);
        if (isBoxedSlotType(vType))
            emitValueDestroy(s, s.stackDepth - 1);
        cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), result);
        s.slotTypes.push_back(SlotType::BOOL);
        return true;
    }

    // INSTANCEOF_TYPEPARAM resolves type-param name through CallFrame::typeArgBindings
    // before the name-based instanceof check. INSTANCEOF (concrete RHS) keeps the
    // cheaper jit_instanceof helper — different signatures, can't share emit code.
    bool emitInstanceofTypeParamOp(JitEmissionState& s,
                                    const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        uint32_t paramNameIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
        SlotType vType = popType(s);
        Gp valAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, vType);
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(paramNameIndex));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_instanceof_typeparam),
                  FuncSignature::build<int64_t, const value::Value*,
                      JitContext*, uint32_t>());
        inv->set_arg(0, valAddr);
        inv->set_arg(1, s.ctxPtr);
        inv->set_arg(2, idx);
        Gp result = cc.new_gp64();
        inv->set_ret(0, result);
        if (isBoxedSlotType(vType))
            emitValueDestroy(s, s.stackDepth - 1);
        cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), result);
        s.slotTypes.push_back(SlotType::BOOL);
        return true;
    }

    bool emitBindTypeArgsOp(JitEmissionState& s,
                            const bytecode::BytecodeProgram::Instruction& /*instr*/)
    {
        // Stage type-arg bindings into ExecutionContext::pendingTypeArgs. The
        // helper reads the instruction's operand vector by IP from the
        // BytecodeProgram, so we just need to pass the current IP.
        auto& cc = s.cc;
        Gp ipReg = cc.new_gp64();
        cc.mov(ipReg, static_cast<int64_t>(s.currentIP));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_bind_type_args),
                  FuncSignature::build<void, JitContext*, uint64_t>());
        inv->set_arg(0, s.ctxPtr);
        inv->set_arg(1, ipReg);
        return true;
    }

    bool emitCastTypeParamOp(JitEmissionState& s,
                              const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t paramNameIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
        SlotType srcType = popType(s);
        Gp srcAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, srcType);
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(paramNameIndex));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_cast_typeparam),
                  FuncSignature::build<void, value::Value*, const value::Value*,
                      JitContext*, uint32_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, srcAddr);
        inv->set_arg(2, s.ctxPtr);
        inv->set_arg(3, idx);
        s.slotTypes.push_back(SlotType::BOXED);
        return true;
    }

    bool emitCastOp(JitEmissionState& s,
                    const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t typeIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
        SlotType srcType = popType(s);
        Gp srcAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, srcType);
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
        Gp pPtr = cc.new_gp64();
        cc.mov(pPtr, s.progPtr);
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(typeIndex));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_cast),
                  FuncSignature::build<void, value::Value*, const value::Value*,
                      const vm::bytecode::BytecodeProgram*, uint32_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, srcAddr);
        inv->set_arg(2, pPtr);
        inv->set_arg(3, idx);
        s.slotTypes.push_back(SlotType::BOXED);
        return true;
    }

    // Structural-equality fused opcodes — direct call to the JIT helper that
    // reads raw int fields and computes hash / equality without per-field
    // operand-stack churn. The slot-indices array is passed as a raw pointer
    // immediate; inline operands aren't stable across vector reallocation, so
    // we materialize a program-owned stable copy.
    bool emitStructHashIntOp(JitEmissionState& s,
                              const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        if (instr.numOperands() == 0) { s.compileFailed = true; return true; }
        const uint64_t fieldCount = instr.inlineOperands[0];
        if (instr.numOperands() < 1 + fieldCount)
        {
            s.compileFailed = true;
            return true;
        }
        const uint64_t* slotsPtr = s.program.materializeStableOperandSlice(
            instr, 1, static_cast<size_t>(fieldCount));

        Gp receiverAddr = cc.new_gp64();
        cc.lea(receiverAddr, Mem(s.boxedBase,
            static_cast<int32_t>((s.stackDepth - 1) * valueSize)));

        Gp slotsImm = cc.new_gp64();
        cc.mov(slotsImm, reinterpret_cast<uint64_t>(slotsPtr));

        Gp result = cc.new_gp64();
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_struct_hash_int),
                  FuncSignature::build<int64_t, const value::Value*,
                                        uint64_t, const uint64_t*>());
        inv->set_arg(0, receiverAddr);
        inv->set_arg(1, fieldCount);
        inv->set_arg(2, slotsImm);
        inv->set_ret(0, result);

        popType(s);
        s.slotTypes.push_back(SlotType::INT);
        publishGpHint(s, s.stackDepth - 1, result);
        return true;
    }

    bool emitStructEqIntOp(JitEmissionState& s,
                            const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        if (instr.numOperands() < 2) { s.compileFailed = true; return true; }
        const uint64_t classNameIdx = instr.inlineOperands[0];
        const uint64_t fieldCount = instr.inlineOperands[1];
        if (instr.numOperands() < 2 + fieldCount)
        {
            s.compileFailed = true;
            return true;
        }
        const uint64_t* slotsPtr = s.program.materializeStableOperandSlice(
            instr, 2, static_cast<size_t>(fieldCount));

        // Class-name string lives in the ConstantPool; storage is stable for
        // the program's lifetime, so a c_str() pointer survives until the
        // helper returns.
        const std::string& className =
            s.program.getConstantPool().getString(classNameIdx);

        // Stack: ..., this (depth-2), other (depth-1).
        Gp thisAddr = cc.new_gp64();
        cc.lea(thisAddr, Mem(s.boxedBase,
            static_cast<int32_t>((s.stackDepth - 2) * valueSize)));
        Gp otherAddr = cc.new_gp64();
        cc.lea(otherAddr, Mem(s.boxedBase,
            static_cast<int32_t>((s.stackDepth - 1) * valueSize)));

        Gp classNameImm = cc.new_gp64();
        cc.mov(classNameImm, reinterpret_cast<uint64_t>(className.c_str()));

        Gp slotsImm = cc.new_gp64();
        cc.mov(slotsImm, reinterpret_cast<uint64_t>(slotsPtr));

        Gp result = cc.new_gp64();
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_struct_eq_int),
                  FuncSignature::build<int64_t, const value::Value*,
                                        const value::Value*, const char*,
                                        uint64_t, const uint64_t*>());
        inv->set_arg(0, thisAddr);
        inv->set_arg(1, otherAddr);
        inv->set_arg(2, classNameImm);
        inv->set_arg(3, fieldCount);
        inv->set_arg(4, slotsImm);
        inv->set_ret(0, result);

        s.stackDepth--;
        popType(s); // other
        popType(s); // this
        s.slotTypes.push_back(SlotType::BOOL);
        publishGpHint(s, s.stackDepth - 1, result);
        return true;
    }
}
