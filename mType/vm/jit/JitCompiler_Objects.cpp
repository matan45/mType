#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    void emitBoxCallArgs(JitEmissionState& s, size_t argCount, size_t destStartSlot)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        for (size_t i = 0; i < argCount; ++i)
        {
            int stackIdx = s.stackDepth - static_cast<int>(argCount) + static_cast<int>(i);
            SlotType argType = (stackIdx >= 0 && stackIdx < static_cast<int>(s.slotTypes.size()))
                             ? s.slotTypes[stackIdx] : SlotType::INT;
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)
                                  + static_cast<int32_t>((i + destStartSlot) * valueSize)));

            if (s.usesBoxedTypes && isBoxedSlotType(argType))
            {
                Gp srcAddr = cc.new_gp64();
                cc.lea(srcAddr, Mem(s.boxedBase, static_cast<int32_t>(stackIdx * valueSize)));
                InvokeNode* cpInv;
                cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                          FuncSignature::build<void, value::Value*, const value::Value*>());
                cpInv->set_arg(0, destAddr);
                cpInv->set_arg(1, srcAddr);
            }
            else
            {
                emitBox(s, destAddr, stackIdx, argType);
            }
        }
    }

    void emitPopAndDestroyArgs(JitEmissionState& s, size_t argCount)
    {
        // popType pops from top-to-bottom, so match with top-to-bottom indices
        for (size_t i = 0; i < argCount; ++i)
        {
            SlotType at = popType(s);
            if (s.usesBoxedTypes && isBoxedSlotType(at))
            {
                // i=0 is the topmost arg (at stackDepth-1), i=1 is next, etc.
                int idx = s.stackDepth - 1 - static_cast<int>(i);
                emitValueDestroy(s, idx);
            }
        }
        s.stackDepth -= static_cast<int>(argCount);
    }

    static bool emitPushStringOp(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t constIndex = static_cast<uint32_t>(instr.operands[0]);
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        Gp pPtr = cc.new_gp64();
        cc.mov(pPtr, s.progPtr);
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(constIndex));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_push_string),
                  FuncSignature::build<void, value::Value*,
                      const vm::bytecode::BytecodeProgram*, uint32_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, pPtr);
        inv->set_arg(2, idx);
        s.slotTypes.push_back(SlotType::STRING);
        s.stackDepth++;
        return true;
    }

    static bool emitGetFieldOp(JitEmissionState& s,
                                const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t fieldNameIndex = static_cast<uint32_t>(instr.operands[0]);
        Gp objAddr = cc.new_gp64();
        cc.lea(objAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
        Gp ipReg = cc.new_gp64();
        cc.mov(ipReg, static_cast<int64_t>(s.currentIP));
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(fieldNameIndex));
        Gp flagsReg = cc.new_gp64();
        cc.mov(flagsReg, static_cast<int64_t>(instr.flags));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_get_field_ic),
                  FuncSignature::build<void, value::Value*, const value::Value*,
                      JitContext*, size_t, uint32_t, uint8_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, objAddr);
        inv->set_arg(2, s.ctxPtr);
        inv->set_arg(3, ipReg);
        inv->set_arg(4, idx);
        inv->set_arg(5, flagsReg);
        popType(s);
        s.slotTypes.push_back(SlotType::BOXED);
        return true;
    }

    static bool emitSetFieldOp(JitEmissionState& s,
                                const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t fieldNameIndex = static_cast<uint32_t>(instr.operands[0]);
        SlotType valType = popType(s);
        popType(s);
        Gp newValAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, valType);
        Gp objAddr = cc.new_gp64();
        cc.lea(objAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));
        Gp ipReg = cc.new_gp64();
        cc.mov(ipReg, static_cast<int64_t>(s.currentIP));
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(fieldNameIndex));
        Gp flagsReg = cc.new_gp64();
        cc.mov(flagsReg, static_cast<int64_t>(instr.flags));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_field_ic),
                  FuncSignature::build<void, value::Value*, const value::Value*,
                      const value::Value*,
                      JitContext*, size_t, uint32_t, uint8_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, objAddr);
        inv->set_arg(2, newValAddr);
        inv->set_arg(3, s.ctxPtr);
        inv->set_arg(4, ipReg);
        inv->set_arg(5, idx);
        inv->set_arg(6, flagsReg);
        if (isBoxedSlotType(valType))
            emitValueDestroy(s, s.stackDepth - 1);
        s.stackDepth--;
        s.slotTypes.push_back(SlotType::BOXED);
        return true;
    }

    static bool emitCallStaticOp(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        uint32_t nameIndex = static_cast<uint32_t>(instr.operands[0]);
        size_t argCount = instr.operands[1];
        if (argCount > JitContext::MAX_CALL_ARGS)
        {
            s.compileFailed = true;
            return true;
        }
        emitBoxCallArgs(s, argCount);
        emitPopAndDestroyArgs(s, argCount);
        Gp niReg = cc.new_gp64();
        cc.mov(niReg, static_cast<int64_t>(nameIndex));
        Gp acReg = cc.new_gp64();
        cc.mov(acReg, static_cast<int64_t>(argCount));
        InvokeNode* callInv;
        cc.invoke(Out(callInv), reinterpret_cast<uint64_t>(jit_call_function),
                  FuncSignature::build<void, JitContext*, uint32_t, size_t>());
        callInv->set_arg(0, s.ctxPtr);
        callInv->set_arg(1, niReg);
        callInv->set_arg(2, acReg);
        emitReturnValueCopyBoxed(s);
        return true;
    }

    static bool emitCallMethodOp(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t methodNameIndex = static_cast<uint32_t>(instr.operands[0]);
        size_t argCount = instr.operands[1];
        if (argCount + 1 > JitContext::MAX_CALL_ARGS)
        {
            s.compileFailed = true;
            return true;
        }
        {
            int objIdx = s.stackDepth - static_cast<int>(argCount) - 1;
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)));
            Gp srcAddr = cc.new_gp64();
            cc.lea(srcAddr, Mem(s.boxedBase, static_cast<int32_t>(objIdx * valueSize)));
            InvokeNode* cpInv;
            cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            cpInv->set_arg(0, destAddr);
            cpInv->set_arg(1, srcAddr);
        }
        emitBoxCallArgs(s, argCount, 1);
        for (size_t i = 0; i < argCount; ++i)
        {
            SlotType at = popType(s);
            if (isBoxedSlotType(at))
                emitValueDestroy(s, s.stackDepth - static_cast<int>(argCount) + static_cast<int>(i));
        }
        popType(s);
        emitValueDestroy(s, s.stackDepth - static_cast<int>(argCount) - 1);
        s.stackDepth -= static_cast<int>(argCount) + 1;
        Gp ipReg = cc.new_gp64();
        cc.mov(ipReg, static_cast<int64_t>(s.currentIP));
        Gp miReg = cc.new_gp64();
        cc.mov(miReg, static_cast<int64_t>(methodNameIndex));
        Gp acReg = cc.new_gp64();
        cc.mov(acReg, static_cast<int64_t>(argCount));
        InvokeNode* callInv;
        cc.invoke(Out(callInv), reinterpret_cast<uint64_t>(jit_call_method_ic),
                  FuncSignature::build<void, JitContext*, size_t, uint32_t, size_t>());
        callInv->set_arg(0, s.ctxPtr);
        callInv->set_arg(1, ipReg);
        callInv->set_arg(2, miReg);
        callInv->set_arg(3, acReg);
        emitReturnValueCopyBoxed(s);
        return true;
    }

    // MYT-147: iterator protocol emitters. Shape A - each opcode routes to a
    // dedicated runtime helper that calls vm->callMethodFromJit with a fixed
    // method name. Receiver is marshalled into ctx->callArgs[0] before calling.
    //
    // CRITICAL - peek vs pop semantics:
    //   GET_ITERATOR   pops receiver, pushes iterator. Net: replace receiver.
    //   HAS_NEXT/NEXT  PEEK receiver (do NOT decrement stackDepth or popType).
    //                  Copy into callArgs[0] fresh each call (a stale pointer
    //                  into boxedBase would double-free when the slot is
    //                  overwritten). Push bool / element on top of the peeked
    //                  iterator. Matches ObjectExecutor::handleIteratorHasNext
    //                  and handleIteratorNext at ObjectExecutor.cpp:976, 999.
    //   CLOSE          pops receiver, no push. Helper swallows exceptions per
    //                  ObjectExecutor.cpp:1040-1048.

    static void emitCopyReceiverToCallArgs(JitEmissionState& s, int receiverStackIdx)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        Gp destAddr = cc.new_gp64();
        cc.lea(destAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)));
        Gp srcAddr = cc.new_gp64();
        cc.lea(srcAddr, Mem(s.boxedBase, static_cast<int32_t>(receiverStackIdx * valueSize)));
        InvokeNode* cpInv;
        cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                  FuncSignature::build<void, value::Value*, const value::Value*>());
        cpInv->set_arg(0, destAddr);
        cpInv->set_arg(1, srcAddr);
    }

    static void emitDestroyCallArg0(JitEmissionState& s)
    {
        auto& cc = s.cc;
        Gp addr = cc.new_gp64();
        cc.lea(addr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_destroy),
                  FuncSignature::build<void, value::Value*>());
        inv->set_arg(0, addr);
    }

    static bool emitGetIteratorOp(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        int receiverIdx = s.stackDepth - 1;
        emitCopyReceiverToCallArgs(s, receiverIdx);
        // Destroy the original receiver slot, pop its type. Result slot will
        // reuse the same position (net stackDepth change is 0).
        emitValueDestroy(s, receiverIdx);
        popType(s);
        s.stackDepth--;

        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_iterator_get),
                  FuncSignature::build<void, value::Value*, JitContext*>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);

        emitDestroyCallArg0(s);

        s.slotTypes.push_back(SlotType::BOXED);
        s.stackDepth++;
        return true;
    }

    static bool emitIteratorHasNextOp(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        // PEEK: receiver stays on stack. Fresh copy into callArgs[0] per call
        // so the shared_ptr refcount management is local to this invocation.
        int receiverIdx = s.stackDepth - 1;
        emitCopyReceiverToCallArgs(s, receiverIdx);

        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_iterator_has_next),
                  FuncSignature::build<void, value::Value*, JitContext*>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);

        // Unbox the bool into stackBase so JUMP_IF_FALSE / JUMP_IF_TRUE can
        // consume it from the primitive stack (matches GET_FIELD / ARRAY_GET
        // mirror-to-stackBase pattern).
        Gp unboxAddr = cc.new_gp64();
        cc.lea(unboxAddr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        InvokeNode* unbox;
        cc.invoke(Out(unbox), reinterpret_cast<uint64_t>(jit_unbox_int),
                  FuncSignature::build<int64_t, const value::Value*>());
        unbox->set_arg(0, unboxAddr);
        Gp unboxed = cc.new_gp64();
        unbox->set_ret(0, unboxed);
        cc.mov(Mem(s.stackBase, s.stackDepth * 8), unboxed);

        emitDestroyCallArg0(s);

        // Destroy the boxed mirror now that we've extracted the bool primitive.
        emitValueDestroy(s, s.stackDepth);

        s.slotTypes.push_back(SlotType::BOOL);
        s.stackDepth++;
        return true;
    }

    static bool emitIteratorNextOp(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        // PEEK: receiver stays on stack.
        int receiverIdx = s.stackDepth - 1;
        emitCopyReceiverToCallArgs(s, receiverIdx);

        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_iterator_next),
                  FuncSignature::build<void, value::Value*, JitContext*>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);

        emitDestroyCallArg0(s);

        s.slotTypes.push_back(SlotType::BOXED);
        s.stackDepth++;
        return true;
    }

    static bool emitIteratorCloseOp(JitEmissionState& s)
    {
        auto& cc = s.cc;

        int receiverIdx = s.stackDepth - 1;
        emitCopyReceiverToCallArgs(s, receiverIdx);
        emitValueDestroy(s, receiverIdx);
        popType(s);
        s.stackDepth--;

        InvokeNode* inv;
        s.cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_iterator_close),
                    FuncSignature::build<void, JitContext*>());
        inv->set_arg(0, s.ctxPtr);

        emitDestroyCallArg0(s);
        return true;
    }

    static bool emitInstanceofOp(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        uint32_t typeIndex = static_cast<uint32_t>(instr.operands[0]);
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

    static bool emitCastOp(JitEmissionState& s,
                            const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t typeIndex = static_cast<uint32_t>(instr.operands[0]);
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

    static bool emitNewObjectOp(JitEmissionState& s,
                                 const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t classIndex = static_cast<uint32_t>(instr.operands[0]);
        size_t argCount = instr.operands[1];
        if (argCount > JitContext::MAX_CALL_ARGS)
        {
            s.compileFailed = true;
            return true;
        }
        emitBoxCallArgs(s, argCount);
        emitPopAndDestroyArgs(s, argCount);
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        Gp ciReg = cc.new_gp64();
        cc.mov(ciReg, static_cast<int64_t>(classIndex));
        Gp acReg = cc.new_gp64();
        cc.mov(acReg, static_cast<int64_t>(argCount));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_new_object),
                  FuncSignature::build<void, value::Value*, JitContext*,
                      uint32_t, size_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);
        inv->set_arg(2, ciReg);
        inv->set_arg(3, acReg);
        s.slotTypes.push_back(SlotType::OBJECT);
        s.stackDepth++;
        return true;
    }

    bool emitObjectOps(JitEmissionState& s,
                       const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        if (emitArrayOps(s, instr)) return true;

        switch (instr.opcode)
        {
            case OpCode::PUSH_STRING:    return emitPushStringOp(s, instr);
            case OpCode::GET_FIELD:      return emitGetFieldOp(s, instr);
            case OpCode::SET_FIELD:      return emitSetFieldOp(s, instr);
            case OpCode::CALL_STATIC:    return emitCallStaticOp(s, instr);
            case OpCode::CALL_METHOD:    return emitCallMethodOp(s, instr);
            case OpCode::INSTANCEOF:     return emitInstanceofOp(s, instr);
            case OpCode::CAST:           return emitCastOp(s, instr);
            case OpCode::NEW_OBJECT:
            case OpCode::NEW_VALUE_OBJECT: return emitNewObjectOp(s, instr);

            case OpCode::GET_ITERATOR:       return emitGetIteratorOp(s);
            case OpCode::ITERATOR_HAS_NEXT:  return emitIteratorHasNextOp(s);
            case OpCode::ITERATOR_NEXT:      return emitIteratorNextOp(s);
            case OpCode::ITERATOR_CLOSE:     return emitIteratorCloseOp(s);

            case OpCode::OBJECT_TO_VALUE:
            {
                Gp addr = cc.new_gp64();
                cc.lea(addr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_object_to_value),
                          FuncSignature::build<void, value::Value*>());
                inv->set_arg(0, addr);
                popType(s);
                s.slotTypes.push_back(SlotType::VALUE_OBJECT);
                return true;
            }

            default:
                return false;
        }
    }
}
