#include "JitCompiler_ControlFlow.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include "../runtime/VirtualMachine.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    namespace
    {
        std::string resolveCallReturnType(JitEmissionState& s,
                                           const bytecode::BytecodeProgram::Instruction& instr)
        {
            uint32_t nameIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
            if (nameIndex >= s.program.getConstantPool().strings.size())
            {
                s.compileFailed = true;
                return "";
            }
            const std::string& funcName = s.program.getConstantPool().getString(nameIndex);
            const auto* calleeMeta = s.program.getFunction(funcName);

            if (calleeMeta)
                return calleeMeta->returnType;

            if (funcName == "print" || funcName == "println")
                return "void";

            s.compileFailed = true;
            return "";
        }

        // Phase 1: lower a self-recursive tail call (CALL/CALL_FAST immediately
        // followed by RETURN_VALUE) to an argument-overwrite + jmp to the
        // function's entry label. Eliminates the per-call dispatch plumbing
        // (jit_call_function, JitContext construction, CallFrame push, try/
        // catch) for self-recursive tail calls — e.g. gcd in recursive.mt
        // collapses from 50k recursive invocations into a tight loop.
        //
        // Returns true iff the call site was lowered. False → caller falls
        // through to the generic helper invoke unchanged.
        //
        // Preconditions:
        //   - callee is the currently compiling function (self-recursion)
        //   - next instruction is RETURN_VALUE, and that RETURN_VALUE is not
        //     a jump target (so lowering leaves it as dead-code-after-jmp)
        //   - frame is !usesBoxedTypes and all params are primitive (int/
        //     float/bool); boxed-frame TCO is deferred (requires ref-count-
        //     correct local overwrite via emitInlineLocalCopy)
        //   - argCount matches parameterCount, and the top-of-stack arg
        //     SlotTypes match the declared param types exactly
        bool tryEmitSelfTailCall(JitEmissionState& s,
                                  size_t argCount,
                                  const bytecode::BytecodeProgram::FunctionMetadata* calleeMeta)
        {
            if (!s.selfTailCallEnabled) return false;
            if (s.usesBoxedTypes) return false;
            if (!s.inlineStack.empty()) return false;
            if (!calleeMeta) return false;
            if (argCount != calleeMeta->parameterCount) return false;
            if (s.currentCompilingFn.empty()) return false;
            // MYT-211: arg copy below reads stackBase memory; flush any cached
            // slots so the copy sees the correct values. The cc.jmp at the
            // end also requires memory-coherent state at the back-edge target.
            flushAllHints(s);

            const std::string& calleeKey = calleeMeta->mangledName.empty()
                ? calleeMeta->name : calleeMeta->mangledName;
            if (calleeKey != s.currentCompilingFn &&
                calleeMeta->name != s.currentCompilingFn &&
                calleeMeta->mangledName != s.currentCompilingFn)
                return false;

            // The next IP must be inside the currently compiling function's
            // bytecode range — otherwise we'd be peeking at the following
            // function's first instruction.
            const size_t nextIP = s.currentIP + 1;
            const size_t funcEnd = calleeMeta->startOffset + calleeMeta->instructionCount;
            if (nextIP >= funcEnd) return false;
            const auto& next = s.program.getInstruction(nextIP);
            if (next.opcode != OpCode::RETURN_VALUE) return false;
            if (s.labels.find(nextIP) != s.labels.end()) return false;

            if (s.slotTypes.size() < argCount) return false;
            if (static_cast<size_t>(s.stackDepth) < argCount) return false;
            const size_t firstArgIdx = static_cast<size_t>(s.stackDepth) - argCount;

            for (size_t i = 0; i < argCount; ++i)
            {
                SlotType expected = SlotType::INT;
                if (i < calleeMeta->parameterTypes.size())
                {
                    const std::string& t = calleeMeta->parameterTypes[i];
                    if (t == "float")      expected = SlotType::FLOAT;
                    else if (t == "bool")  expected = SlotType::BOOL;
                    else if (t != "int")   return false;  // BOXED param: defer.
                }
                if (s.slotTypes[firstArgIdx + i] != expected) return false;
            }

            // Return type must be primitive so the phantom slot-type we push
            // below (for the dead-code RETURN_VALUE) stays consistent with
            // the function's declared return shape.
            SlotType retSlot = SlotType::INT;
            if (calleeMeta->returnType == "float")      retSlot = SlotType::FLOAT;
            else if (calleeMeta->returnType == "bool")  retSlot = SlotType::BOOL;
            else if (calleeMeta->returnType != "int")   return false;

            auto& cc = s.cc;

            // Copy stackBase[firstArgIdx + i] → localsBase[i] for each arg.
            // localStride == 8 in !usesBoxedTypes mode — primitive slots only,
            // no ref-count traffic needed.
            for (size_t i = 0; i < argCount; ++i)
            {
                const int32_t srcOff = static_cast<int32_t>((firstArgIdx + i) * 8);
                const int32_t dstOff = static_cast<int32_t>(i * 8);
                SlotType pt = s.slotTypes[firstArgIdx + i];
                if (pt == SlotType::FLOAT)
                {
                    Vec v = cc.new_xmm();
                    cc.movsd(v, Mem(s.stackBase, srcOff));
                    cc.movsd(Mem(s.localsBase, dstOff), v);
                }
                else
                {
                    Gp reg = cc.new_gp64();
                    cc.mov(reg, Mem(s.stackBase, srcOff));
                    cc.mov(Mem(s.localsBase, dstOff), reg);
                }
            }

            // Logical state after the tail call: argCount args consumed, 1
            // return-typed value pushed. The following (dead) RETURN_VALUE
            // will pop this phantom slot to stay consistent at the asmjit
            // level — runtime never reaches it, the jmp below leaves the block.
            for (size_t i = 0; i < argCount; ++i) popType(s);
            s.stackDepth -= static_cast<int>(argCount);
            s.slotTypes.push_back(retSlot);
            s.stackDepth++;

            // MYT-226: per-frame tail-call counter overflow check.
            Label tailOverflowLabel = cc.new_label();
            Gp vmReg = cc.new_gp64();
            cc.mov(vmReg, Mem(s.ctxPtr, offsetof(JitContext, vm)));
            Gp maxReg = cc.new_gp64();
            cc.mov(maxReg, Mem(vmReg, vm::runtime::VirtualMachine::kMaxCallStackSizeOffset));
            cc.inc(s.tailCallCounter);
            cc.cmp(s.tailCallCounter, maxReg);
            cc.jae(tailOverflowLabel);

            emitInlineGcSafepoint(s);

            cc.jmp(s.functionEntryLabel);

            // MYT-226: throw path — dead fall-through from the jmp above.
            cc.bind(tailOverflowLabel);
            InvokeNode* throwInv;
            cc.invoke(Out(throwInv),
                      reinterpret_cast<uint64_t>(jit_throw_stack_overflow),
                      FuncSignature::build<void, size_t>());
            throwInv->set_arg(0, maxReg);
            // helper does not return; asmjit handles the unwind.

            // MYT-207 telemetry: compile-time bump (not runtime). Counts the
            // number of CALL sites lowered to a tail-loop, NOT the number of
            // dynamic iterations the loop runs.
            if (s.tailCallsOptimized) (*s.tailCallsOptimized)++;
            return true;
        }

        // MYT-207: lower a NON-tail self-recursive CALL/CALL_FAST to a direct
        // asmjit invoke against the function's own FuncNode->label(). Skips
        // the jit_call_function dispatch overhead — concretely:
        //   - jitCodeCache lookupByIndex (we ARE the lookup target)
        //   - tryJitDispatchResolved's CallFrame push + std::vector grow
        //   - JitContext nestedCtx{} zero-init (~256 bytes / 16 variant defaults)
        //   - try/catch SEH frame setup
        // The dominant residual cost on recursive.mt's fib (~2.76M non-tail
        // recursive calls) is exactly that overhead, since fib's body is tiny.
        //
        // Reuses the same JitContext: args are marshalled into ctx->callArgs
        // by emitBoxCallArgs (mirrors the generic path), then ctx->args is
        // pointed at &ctx->callArgs[0] for the duration of the nested invoke
        // and restored afterwards. ctx->returnValue / ctx->hasReturnValue are
        // populated by the callee's RETURN_VALUE.
        //
        // Stack-overflow protection: inline cmp against
        // VirtualMachine::MAX_JIT_NATIVE_DEPTH; on overflow the emission
        // branches to a fallback path that calls the original generic
        // helper, which itself bails to the interpreter via
        // callFunctionFromJit. Tail-optimized frames do NOT appear in stack
        // traces — same accepted semantic as MYT-210's inliner.
        bool tryEmitSelfDirectCall(JitEmissionState& s,
                                    size_t argCount,
                                    const bytecode::BytecodeProgram::FunctionMetadata* calleeMeta,
                                    uint64_t fallbackHelper,
                                    uint32_t fallbackArg)
        {
            if (!s.selfDirectCallEnabled) return false;
            if (s.usesBoxedTypes) return false;
            if (!calleeMeta) return false;
            if (argCount != calleeMeta->parameterCount) return false;
            if (s.currentCompilingFn.empty()) return false;
            // MYT-211: cc.invoke and emitBoxCallArgs read stack memory.
            flushAllHints(s);

            const std::string& calleeKey = calleeMeta->mangledName.empty()
                ? calleeMeta->name : calleeMeta->mangledName;
            if (calleeKey != s.currentCompilingFn &&
                calleeMeta->name != s.currentCompilingFn &&
                calleeMeta->mangledName != s.currentCompilingFn)
                return false;

            // Return must be a primitive in unboxed-frame mode (mirrors the
            // outer emit-path's check at emitCallOp / emitCallFastOp). Non-
            // primitive returns require boxed-frame plumbing which v1 defers.
            const std::string& returnType = calleeMeta->returnType;
            const bool isPrimReturn = (returnType == "int" || returnType == "float" ||
                                       returnType == "bool" || returnType == "void");
            if (!isPrimReturn) return false;

            auto& cc = s.cc;

            // Marshal args from operand stack → ctx->callArgs[0..argCount-1]
            // and pop the operand-stack slot types. Same helper the generic
            // path uses; both the direct invoke and the fallback consume
            // callArgs.
            emitBoxCallArgs(s, argCount);
            emitPopAndDestroyArgs(s, argCount);

            // Load vmPtr once; both the depth-guard and the inc/dec touch it.
            Gp vmPtr = cc.new_gp64();
            cc.mov(vmPtr, Mem(s.ctxPtr, offsetof(JitContext, vm)));

            // Inline depth guard. On overflow, branch to fallbackLabel which
            // emits the generic helper (which itself bails to the interpreter
            // via callFunctionFromJit when the depth check trips again).
            Label fallbackLabel = cc.new_label();
            Label afterCallLabel = cc.new_label();

            Gp depth = cc.new_gp64();
            cc.mov(depth, Mem(vmPtr, static_cast<int32_t>(
                vm::runtime::VirtualMachine::kJitNativeDepthOffset)));
            cc.cmp(depth, static_cast<int32_t>(
                vm::runtime::VirtualMachine::MAX_JIT_NATIVE_DEPTH));
            cc.jae(fallbackLabel);

            // ---- Direct path ----

            // Save current ctx->args / argCount so the outer caller's view is
            // unchanged after the recursive call returns. The callee's
            // prologue reads ctx->args once (emitArgumentUnboxing) and copies
            // into its locals; we point at our own callArgs[] for that read.
            Gp savedArgs = cc.new_gp64();
            Gp savedArgCount = cc.new_gp64();
            cc.mov(savedArgs, Mem(s.ctxPtr, offsetof(JitContext, args)));
            cc.mov(savedArgCount, Mem(s.ctxPtr, offsetof(JitContext, argCount)));

            Gp callArgsPtr = cc.new_gp64();
            cc.lea(callArgsPtr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)));
            cc.mov(Mem(s.ctxPtr, offsetof(JitContext, args)), callArgsPtr);
            Gp argCountReg = cc.new_gp64();
            cc.mov(argCountReg, static_cast<int64_t>(argCount));
            cc.mov(Mem(s.ctxPtr, offsetof(JitContext, argCount)), argCountReg);

            // Inc native depth before the call (mirrors tryJitDispatchResolved).
            cc.inc(depth);
            cc.mov(Mem(vmPtr, static_cast<int32_t>(
                vm::runtime::VirtualMachine::kJitNativeDepthOffset)), depth);

            // Direct invoke against the function's own FuncNode->label().
            // AsmJit resolves the (still-unbound at this point) label at
            // cc.finalize() time; recursive label invokes are supported.
            InvokeNode* selfInv;
            cc.invoke(Out(selfInv), s.selfFuncCallLabel,
                      FuncSignature::build<void, JitContext*>());
            selfInv->set_arg(0, s.ctxPtr);

            // Reload vmPtr/depth — caller-saved registers are clobbered by
            // the call; let asmjit allocate fresh virtregs so the dec uses
            // live data.
            Gp vmPtr2 = cc.new_gp64();
            cc.mov(vmPtr2, Mem(s.ctxPtr, offsetof(JitContext, vm)));
            Gp depth2 = cc.new_gp64();
            cc.mov(depth2, Mem(vmPtr2, static_cast<int32_t>(
                vm::runtime::VirtualMachine::kJitNativeDepthOffset)));
            cc.dec(depth2);
            cc.mov(Mem(vmPtr2, static_cast<int32_t>(
                vm::runtime::VirtualMachine::kJitNativeDepthOffset)), depth2);

            // Restore the caller's view of ctx->args / argCount.
            cc.mov(Mem(s.ctxPtr, offsetof(JitContext, args)), savedArgs);
            cc.mov(Mem(s.ctxPtr, offsetof(JitContext, argCount)), savedArgCount);

            cc.jmp(afterCallLabel);

            // ---- Fallback path: depth exceeded, defer to the generic helper
            cc.bind(fallbackLabel);
            Gp fbArgReg = cc.new_gp64();
            cc.mov(fbArgReg, static_cast<int64_t>(fallbackArg));
            Gp fbAcReg = cc.new_gp64();
            cc.mov(fbAcReg, static_cast<int64_t>(argCount));
            InvokeNode* fbInv;
            cc.invoke(Out(fbInv), fallbackHelper,
                      FuncSignature::build<void, JitContext*, uint32_t, size_t>());
            fbInv->set_arg(0, s.ctxPtr);
            fbInv->set_arg(1, fbArgReg);
            fbInv->set_arg(2, fbAcReg);

            cc.bind(afterCallLabel);

            // Push the return value back onto the operand stack — same path
            // the generic emit takes.
            if (returnType != "void")
                emitCallReturnValue(s, returnType, /*isPrimReturn=*/true);

            if (s.selfDirectCalls) (*s.selfDirectCalls)++;
            return true;
        }
    }

    bool emitCallOp(JitEmissionState& s,
                    const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        uint32_t nameIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
        size_t argCount = instr.inlineOperands[1];

        if (argCount > JitContext::MAX_CALL_ARGS)
        {
            s.compileFailed = true;
            return true;
        }

        // Phase 1: self-recursive tail-call optimization. Checked before
        // the inliner because self-recursion is rejected by the inliner
        // (InlineAnalysis.cpp:141-145 SELF_RECURSIVE), so TCO is the only
        // opportunity to elide the call dispatch for recursive functions.
        // MYT-207 Phase 2: non-tail self-recursive direct invoke against
        // the function's own FuncNode->label() — same rationale, targets
        // fib's residual ~2.76M non-tail recursive calls in recursive.mt.
        if (nameIndex < s.program.getConstantPool().strings.size())
        {
            const std::string& funcName = s.program.getConstantPool().getString(nameIndex);
            const auto* calleeMeta = s.program.getFunction(funcName);
            if (tryEmitSelfTailCall(s, argCount, calleeMeta))
                return true;
            if (tryEmitSelfDirectCall(s, argCount, calleeMeta,
                                       reinterpret_cast<uint64_t>(jit_call_function),
                                       nameIndex))
                return true;

            // MYT-316: try speculative inlining before falling through to
            // the cc.invoke runtime boundary. The inliner enforces its own
            // eligibility gates (size, opcode deny-list, depth, stack peak);
            // if it declines, we fall through to the generic dispatch below.
            if (calleeMeta)
            {
                const auto calleeHandle = s.program.internFrameName(funcName);
                if (tryEmitInlinedFunctionCall(s, calleeMeta, argCount, calleeHandle))
                    return true;
            }
        }

        std::string returnType = resolveCallReturnType(s, instr);
        if (s.compileFailed) return true;

        bool isPrimReturn = (returnType == "int" || returnType == "float" ||
                             returnType == "bool" || returnType == "void");
        if (!isPrimReturn && !s.usesBoxedTypes)
        {
            s.compileFailed = true;
            return true;
        }

        // MYT-211: emitBoxCallArgs reads stackBase memory.
        flushAllHints(s);
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

        if (returnType != "void")
            emitCallReturnValue(s, returnType, isPrimReturn);

        return true;
    }

    bool emitCallFastOp(JitEmissionState& s,
                        const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        uint32_t funcIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
        size_t argCount = instr.inlineOperands[1];

        if (argCount > JitContext::MAX_CALL_ARGS)
        {
            s.compileFailed = true;
            return true;
        }

        const auto* calleeMeta = s.program.getFunctionByIndex(funcIndex);

        // Phase 1: self-recursive tail-call optimization. See emitCallOp
        // for rationale — checked before the inliner.
        // MYT-207 Phase 2: non-tail self-recursive direct invoke.
        if (tryEmitSelfTailCall(s, argCount, calleeMeta))
            return true;
        if (tryEmitSelfDirectCall(s, argCount, calleeMeta,
                                   reinterpret_cast<uint64_t>(jit_call_function_fast),
                                   funcIndex))
            return true;

        if (!calleeMeta)
        {
            s.compileFailed = true;
            return true;
        }

        // MYT-316: speculative inlining before the runtime boundary.
        // CALL_FAST's funcIndex is the stable post-IC dispatch key; the
        // matching handle is interned from the callee's name.
        {
            const auto calleeHandle = s.program.internFrameName(calleeMeta->name);
            if (tryEmitInlinedFunctionCall(s, calleeMeta, argCount, calleeHandle))
                return true;
        }

        const std::string& returnType = calleeMeta->returnType;

        bool isPrimReturn = (returnType == "int" || returnType == "float" ||
                             returnType == "bool" || returnType == "void");
        if (!isPrimReturn && !s.usesBoxedTypes)
        {
            s.compileFailed = true;
            return true;
        }

        // MYT-211: emitBoxCallArgs reads stackBase memory.
        flushAllHints(s);
        emitBoxCallArgs(s, argCount);
        emitPopAndDestroyArgs(s, argCount);

        Gp fiReg = cc.new_gp64();
        cc.mov(fiReg, static_cast<int64_t>(funcIndex));
        Gp acReg = cc.new_gp64();
        cc.mov(acReg, static_cast<int64_t>(argCount));

        InvokeNode* callInv;
        cc.invoke(Out(callInv), reinterpret_cast<uint64_t>(jit_call_function_fast),
                  FuncSignature::build<void, JitContext*, uint32_t, size_t>());
        callInv->set_arg(0, s.ctxPtr);
        callInv->set_arg(1, fiReg);
        callInv->set_arg(2, acReg);

        if (returnType != "void")
            emitCallReturnValue(s, returnType, isPrimReturn);

        return true;
    }
}
