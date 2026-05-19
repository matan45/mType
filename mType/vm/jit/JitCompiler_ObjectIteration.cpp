#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>
#include <cstddef>
#include <cstdint>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    // Iterator-protocol emitters. Each opcode routes to a runtime helper that
    // calls vm->callMethodFromJit with a fixed method name. The receiver is
    // marshalled into ctx->callArgs[0] before each call.
    //
    // Peek vs pop semantics:
    //   GET_ITERATOR   pops receiver, pushes iterator. Net: replace receiver.
    //   HAS_NEXT/NEXT  POP receiver (post-MYT-156, matches handle*Next executors).
    //                  Result slot reuses the receiver's position; net stackDepth
    //                  change is 0.
    //   CLOSE          pops receiver, no push. Helper swallows exceptions.

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

    bool emitGetIteratorOp(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        int receiverIdx = s.stackDepth - 1;
        emitCopyReceiverToCallArgs(s, receiverIdx);
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

    bool emitIteratorHasNextOp(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        int receiverIdx = s.stackDepth - 1;
        emitCopyReceiverToCallArgs(s, receiverIdx);
        emitValueDestroy(s, receiverIdx);
        popType(s);
        s.stackDepth--;

        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_iterator_has_next),
                  FuncSignature::build<void, value::Value*, JitContext*>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);

        // Unbox the bool into stackBase so JUMP_IF_FALSE / JUMP_IF_TRUE can
        // consume it from the primitive stack.
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

        emitValueDestroy(s, s.stackDepth);

        s.slotTypes.push_back(SlotType::BOOL);
        s.stackDepth++;
        return true;
    }

    bool emitIteratorNextOp(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        int receiverIdx = s.stackDepth - 1;
        emitCopyReceiverToCallArgs(s, receiverIdx);
        emitValueDestroy(s, receiverIdx);
        popType(s);
        s.stackDepth--;

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

    bool emitIteratorCloseOp(JitEmissionState& s)
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

    // OBJECT_TO_VALUE: if TOS is already a VALUE_OBJECT, no-op. Otherwise
    // call jit_object_to_value to convert in place.
    bool emitObjectToValueOp(JitEmissionState& s)
    {
        if (topType(s) == SlotType::VALUE_OBJECT)
            return true;

        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
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
}
