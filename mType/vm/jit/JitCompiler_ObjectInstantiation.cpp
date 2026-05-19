#include "JitEmissionState.hpp"
#include "JitCompiler_Objects.hpp"
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

    bool emitNewObjectOp(JitEmissionState& s,
                         const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t classIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
        size_t argCount = instr.inlineOperands[1];
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
        const uint64_t helper =
            (instr.opcode == OpCode::NEW_VALUE_OBJECT)
                ? reinterpret_cast<uint64_t>(jit_new_value_object)
                : reinterpret_cast<uint64_t>(jit_new_object);
        cc.invoke(Out(inv), helper,
                  FuncSignature::build<void, value::Value*, JitContext*,
                      uint32_t, size_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);
        inv->set_arg(2, ciReg);
        inv->set_arg(3, acReg);
        s.slotTypes.push_back(instr.opcode == OpCode::NEW_VALUE_OBJECT
                                  ? SlotType::VALUE_OBJECT
                                  : SlotType::OBJECT);
        s.stackDepth++;
        return true;
    }

    // Dedicated emitter for NEW_STACK. Calls jit_new_stack instead of
    // jit_new_object so the produced Value carries STACK_OBJECT (when the VM
    // hits the trivial-ctor fast path) and avoids the shared_ptr control block
    // + GC register costs.
    bool emitNewStackOp(JitEmissionState& s,
                         const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t classIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
        size_t argCount = instr.inlineOperands[1];
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
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_new_stack),
                  FuncSignature::build<void, value::Value*, JitContext*,
                      uint32_t, size_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);
        inv->set_arg(2, ciReg);
        inv->set_arg(3, acReg);
        s.slotTypes.push_back(SlotType::STACK_OBJECT);
        s.stackDepth++;
        return true;
    }

    bool emitCreatePromiseOp(JitEmissionState& s)
    {
        if (!s.usesBoxedTypes || s.stackDepth <= 0)
        {
            s.compileFailed = true;
            return true;
        }

        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        flushAllHints(s);

        const int stackIdx = s.stackDepth - 1;
        SlotType valueType = popType(s);
        Gp valueAddr = cc.new_gp64();
        cc.lea(valueAddr, Mem(s.boxedBase, static_cast<int32_t>(stackIdx * valueSize)));
        emitBoxOrCopy(s, valueAddr, stackIdx, valueType);

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_create_promise),
                  FuncSignature::build<void, JitContext*, value::Value*>());
        inv->set_arg(0, s.ctxPtr);
        inv->set_arg(1, valueAddr);

        s.slotTypes.push_back(SlotType::BOXED);
        return true;
    }

    bool emitObjectToValueCreatePromiseOp(JitEmissionState& s)
    {
        if (!s.usesBoxedTypes || s.stackDepth <= 0)
        {
            s.compileFailed = true;
            return true;
        }

        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        flushAllHints(s);

        const int stackIdx = s.stackDepth - 1;
        SlotType valueType = popType(s);
        Gp valueAddr = cc.new_gp64();
        cc.lea(valueAddr, Mem(s.boxedBase, static_cast<int32_t>(stackIdx * valueSize)));
        emitBoxOrCopy(s, valueAddr, stackIdx, valueType);

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_object_to_value_create_promise),
                  FuncSignature::build<void, JitContext*, value::Value*>());
        inv->set_arg(0, s.ctxPtr);
        inv->set_arg(1, valueAddr);

        s.slotTypes.push_back(SlotType::BOXED);
        return true;
    }

    // AWAIT is shaped like CREATE_PROMISE: TOS holds the Value to resolve,
    // helper rewrites it in place. The Phase 2a fast path inlines the
    // PROMISE_INT unwrap (the dominant bench case) so it skips both cc.invoke
    // sites. Any non-PROMISE_INT tag falls through to the helper, which still
    // handles fulfilled heap promises and the deopt-stash path.
    //
    // jit_await stashes OSRDeoptException on ctx->pendingException instead of
    // throwing (the throw form crashed silently on Windows x64 — no PE unwind
    // data registered for the asmjit frame). After the cc.invoke we emit a
    // jit_has_pending_exception check and jump to the per-function deopt-exit
    // label on hit. executeCallWithJit / executeCallFastWithJit detect the
    // stashed exception after jitCode returns and route to the interpreter.
    bool emitAwaitOp(JitEmissionState& s)
    {
        if (!s.usesBoxedTypes || s.stackDepth <= 0)
        {
            s.compileFailed = true;
            return true;
        }

        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        flushAllHints(s);

        const int stackIdx = s.stackDepth - 1;
        SlotType valueType = popType(s);
        Gp valueAddr = cc.new_gp64();
        cc.lea(valueAddr, Mem(s.boxedBase, static_cast<int32_t>(stackIdx * valueSize)));
        emitBoxOrCopy(s, valueAddr, stackIdx, valueType);

        Label slowLabel = cc.new_label();
        Label joinLabel = cc.new_label();

        // Phase 2a inline fast path. Load the tag byte at offset 0 and compare
        // to PROMISE_INT. Match: rewrite the tag byte to INT in place — the
        // int64 payload at offset 8 is already the resolved value, no further
        // mutation required. Miss: fall through to the helper.
        Gp tagReg = cc.new_gp32();
        cc.movzx(tagReg, byte_ptr(valueAddr, 0));
        cc.cmp(tagReg, static_cast<int32_t>(value::ValueType::PROMISE_INT));
        cc.jne(slowLabel);
        cc.mov(byte_ptr(valueAddr, 0),
               static_cast<int32_t>(value::ValueType::INT));
        cc.jmp(joinLabel);

        cc.bind(slowLabel);

        Gp ipReg = cc.new_gp64();
        cc.mov(ipReg, static_cast<int64_t>(s.currentIP));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_await),
                  FuncSignature::build<void, JitContext*, value::Value*, uint64_t>());
        inv->set_arg(0, s.ctxPtr);
        inv->set_arg(1, valueAddr);
        inv->set_arg(2, ipReg);

        // Post-invoke deopt-exit check. Lazily allocate the per-function exit
        // label on first AWAIT; emitFunctionBody binds it after emitCodegenLoop.
        //
        // OSR compilation skips this — compileLoopOSR has no emitFunctionBody
        // pass, and the back-edge check at the next iteration detects
        // pendingException and routes through osrExit. Subsequent helpers in
        // the same iteration short-circuit via their own pendingException guard.
        if (!s.isOSRCompilation)
        {
            if (!s.hasAwaitDeoptExit)
            {
                s.functionDeoptExitLabel = cc.new_label();
                s.hasAwaitDeoptExit = true;
            }
            InvokeNode* checkInv = nullptr;
            cc.invoke(Out(checkInv),
                      reinterpret_cast<uint64_t>(jit_has_pending_exception),
                      FuncSignature::build<int64_t, const JitContext*>());
            checkInv->set_arg(0, s.ctxPtr);
            Gp pendingResult = cc.new_gp64();
            checkInv->set_ret(0, pendingResult);
            cc.test(pendingResult, pendingResult);
            cc.jnz(s.functionDeoptExitLabel);
        }

        cc.bind(joinLabel);

        s.slotTypes.push_back(SlotType::BOXED);
        return true;
    }
}
