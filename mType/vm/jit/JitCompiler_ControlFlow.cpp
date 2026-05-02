#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../../gc/GCConfig.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    // Emit an inline GC poll: bump g_jit_gc_poll_counter and only invoke
    // jit_gc_safepoint (which resets the counter and calls GC::maybeCollect)
    // when it crosses GC_CHECK_INTERVAL. Skips the ABI register-spill
    // overhead of cc.invoke on the common path — critical for tight loops
    // where JUMP_BACK fires millions of times.
    static void emitInlineGcSafepoint(JitEmissionState& s)
    {
        auto& cc = s.cc;
        asmjit::x86::Gp cntAddr = cc.new_gp64();
        asmjit::x86::Gp cnt = cc.new_gp64();
        cc.mov(cntAddr, reinterpret_cast<uint64_t>(&g_jit_gc_poll_counter));
        cc.mov(cnt, asmjit::x86::qword_ptr(cntAddr));
        cc.inc(cnt);
        cc.mov(asmjit::x86::qword_ptr(cntAddr), cnt);
        cc.cmp(cnt, static_cast<int32_t>(gc::config::GC_CHECK_INTERVAL));
        asmjit::Label skipLabel = cc.new_label();
        cc.jb(skipLabel);
        asmjit::InvokeNode* inv;
        cc.invoke(asmjit::Out(inv), reinterpret_cast<uint64_t>(jit_gc_safepoint),
                  asmjit::FuncSignature::build<void>());
        cc.bind(skipLabel);
    }

    // MYT-164 (Phase F-b): resolve a jump target against the innermost active
    // inline frame's localJumpLabels. Returns nullptr when no inline frame is
    // active or the target is outside the callee's bytecode range (in which
    // case the JUMP-family emitter falls back to the outer s.labels / onExit
    // path). Only the top frame is consulted — jumps inside an inner callee
    // cannot target an outer frame's bytecode range by lexical scope.
    static asmjit::Label* findInlineJumpLabel(JitEmissionState& s, size_t target)
    {
        if (s.inlineStack.empty()) return nullptr;
        auto& frame = s.inlineStack.back();
        auto it = frame.localJumpLabels.find(target);
        if (it == frame.localJumpLabels.end()) return nullptr;
        return &it->second;
    }

    static void emitLoadLocal(JitEmissionState& s, size_t slot)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        // MYT-163: remap the bytecode's logical slot into the caller's frame
        // when emission is inside an inlined callee. inlineLocalsBase == 0 at
        // the top level — the computation is a no-op for non-inline paths.
        const size_t physSlot = slot + s.inlineLocalsBase;
        SlotType lt = s.localTypes.count(physSlot) ? s.localTypes[physSlot] : SlotType::INT;

        if (s.usesBoxedTypes && isBoxedSlotType(lt))
        {
            flushAllHints(s);
            Gp src = cc.new_gp64();
            cc.lea(src, Mem(s.localsBase, static_cast<int32_t>(physSlot * s.localStride)));
            Gp dst = cc.new_gp64();
            cc.lea(dst, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            inv->set_arg(0, dst);
            inv->set_arg(1, src);
        }
        else if (s.usesBoxedTypes)
        {
            flushAllHints(s);
            Gp localAddr = cc.new_gp64();
            cc.lea(localAddr, Mem(s.localsBase, static_cast<int32_t>(physSlot * s.localStride)));
            if (lt == SlotType::FLOAT)
            {
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_float),
                          FuncSignature::build<double, const value::Value*>());
                inv->set_arg(0, localAddr);
                Vec val = cc.new_xmm();
                inv->set_ret(0, val);
                cc.movsd(Mem(s.stackBase, s.stackDepth * 8), val);
                // MYT-211: record the unboxed primitive in the cache so the
                // next arith/cmp consumer skips re-reading stackBase.
                recordXmmHint(s, s.stackDepth, val);
            }
            else
            {
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_int),
                          FuncSignature::build<int64_t, const value::Value*>());
                inv->set_arg(0, localAddr);
                Gp val = cc.new_gp64();
                inv->set_ret(0, val);
                cc.mov(Mem(s.stackBase, s.stackDepth * 8), val);
                recordGpHint(s, s.stackDepth, val);
            }
        }
        else
        {
            // MYT-211: load to a fresh virtreg, write to stack memory via
            // publishGpHint. With caching disabled, publishGpHint just emits
            // cc.mov(Mem, reg) — same generated code as the previous inline
            // form. The indirection lets a future cache layer record the
            // virtreg without changing this site.
            Gp tmp = cc.new_gp64();
            cc.mov(tmp, Mem(s.localsBase, static_cast<int32_t>(physSlot * 8)));
            s.slotTypes.push_back(lt);
            publishGpHint(s, s.stackDepth, tmp);
            s.stackDepth++;
            return;
        }
        s.slotTypes.push_back(lt);
        s.stackDepth++;
    }

    static void emitStoreLocal(JitEmissionState& s, size_t slot)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        SlotType tt = topType(s);
        // MYT-163: see emitLoadLocal — remap through inlineLocalsBase.
        const size_t physSlot = slot + s.inlineLocalsBase;

        if (s.usesBoxedTypes && isBoxedSlotType(tt))
        {
            flushAllHints(s);
            Gp src = cc.new_gp64();
            cc.lea(src, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
            Gp dst = cc.new_gp64();
            cc.lea(dst, Mem(s.localsBase, static_cast<int32_t>(physSlot * s.localStride)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            inv->set_arg(0, dst);
            inv->set_arg(1, src);
        }
        else if (s.usesBoxedTypes)
        {
            flushAllHints(s);
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(s.localsBase, static_cast<int32_t>(physSlot * s.localStride)));
            if (tt == SlotType::FLOAT)
            {
                Vec val = cc.new_xmm();
                cc.movsd(val, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_box_float),
                          FuncSignature::build<void, value::Value*, double>());
                inv->set_arg(0, destAddr);
                inv->set_arg(1, val);
            }
            else
            {
                Gp val = cc.new_gp64();
                cc.mov(val, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                uint64_t fn = (tt == SlotType::BOOL)
                    ? reinterpret_cast<uint64_t>(jit_box_bool)
                    : reinterpret_cast<uint64_t>(jit_box_int);
                InvokeNode* inv;
                cc.invoke(Out(inv), fn,
                          FuncSignature::build<void, value::Value*, int64_t>());
                inv->set_arg(0, destAddr);
                inv->set_arg(1, val);
            }
        }
        else
        {
            // MYT-211: consume + republish. Now safe in boxed mode after
            // publishGpHint was fixed to always write to stackBase.
            Gp val = consumeGpHint(s, s.stackDepth - 1);
            cc.mov(Mem(s.localsBase, static_cast<int32_t>(physSlot * 8)), val);
            publishGpHint(s, s.stackDepth - 1, val);
        }
        s.localTypes[physSlot] = tt;
    }

    // MYT-152: JIT emitter for LOAD_VAR. Globals and unqualified field lookups
    // have no compile-time primitive type, so the loaded Value is always
    // placed in the boxed operand stack. scanOpcodesForBoxedTypes forces
    // s.usesBoxedTypes = true whenever LOAD_VAR is present in the loop body;
    // the guard below bails cleanly if that invariant is ever violated.
    static void emitLoadVar(JitEmissionState& s, uint32_t nameIndex)
    {
        if (!s.usesBoxedTypes) { s.compileFailed = true; return; }
        // MYT-211: flush is a no-op in boxed mode (cache stays empty there),
        // but kept for symmetry — if the cache invariant changes later this
        // call site stays correct.
        flushAllHints(s);
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase,
                         static_cast<int32_t>(s.stackDepth * valueSize)));
        Gp niReg = cc.new_gp64();
        cc.mov(niReg, static_cast<int64_t>(nameIndex));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_load_var),
                  FuncSignature::build<void, value::Value*, JitContext*,
                                        uint32_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);
        inv->set_arg(2, niReg);

        s.slotTypes.push_back(SlotType::BOXED);
        s.stackDepth++;
    }

    // MYT-152: JIT emitter for STORE_VAR. The runtime pops the value, writes
    // it to the global/field, then re-pushes the same bits (so expression-
    // context `int x = (a = 5)` works). We mirror emitStoreLocal's approach:
    // pass the top slot to jit_store_var and leave stackDepth / slotTypes
    // untouched, since the bits at top-of-stack are identical before and
    // after. The statement-context POP emitted by StatementCompiler's Path B
    // consumes the re-pushed value.
    static void emitStoreVar(JitEmissionState& s, uint32_t nameIndex)
    {
        if (!s.usesBoxedTypes) { s.compileFailed = true; return; }
        flushAllHints(s);
        auto& cc = s.cc;

        SlotType valType = topType(s);
        Gp valAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, valType);
        Gp niReg = cc.new_gp64();
        cc.mov(niReg, static_cast<int64_t>(nameIndex));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_store_var),
                  FuncSignature::build<void, JitContext*, uint32_t,
                                        const value::Value*>());
        inv->set_arg(0, s.ctxPtr);
        inv->set_arg(1, niReg);
        inv->set_arg(2, valAddr);
    }

    // MYT-208: DECLARE_VAR — pop the top-of-stack value and register a new
    // global with that value. Stack effect: -1 (pure pop). The helper's
    // VariableDefinition ctor copies the Value internally; on the JIT side
    // we just need to drop the slot. Boxed slots get an explicit destroy so
    // their heap-ref refcount is released; primitive slots have no heap ref
    // to release and their boxedBase[] entry would be uninitialised.
    static void emitDeclareVar(JitEmissionState& s,
                                const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (!s.usesBoxedTypes) { s.compileFailed = true; return; }
        flushAllHints(s);
        auto& cc = s.cc;
        uint32_t nameIndex = static_cast<uint32_t>(instr.operands[0]);
        // operand[1] is the type-name index (currently unused at JIT level).
        uint8_t isFinal = (instr.operands.size() >= 3 && instr.operands[2] != 0) ? 1 : 0;

        SlotType valType = topType(s);
        Gp valAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, valType);
        Gp niReg = cc.new_gp64();
        cc.mov(niReg, static_cast<int64_t>(nameIndex));
        Gp finalReg = cc.new_gp64();
        cc.mov(finalReg, static_cast<int64_t>(isFinal));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_declare_var),
                  FuncSignature::build<void, JitContext*, uint32_t,
                                        const value::Value*, uint8_t>());
        inv->set_arg(0, s.ctxPtr);
        inv->set_arg(1, niReg);
        inv->set_arg(2, valAddr);
        inv->set_arg(3, finalReg);

        if (isBoxedSlotType(valType))
        {
            emitValueDestroy(s, s.stackDepth - 1);
        }
        popType(s);
        s.stackDepth--;
    }

    static void emitReturnPrimitive(JitEmissionState& s, SlotType retType)
    {
        // MYT-211: return path reads stack memory; flush before emitting the
        // helper invokes so the return value reflects the latest cached state.
        flushAllHints(s);
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        if (s.usesBoxedTypes && isBoxedSlotType(retType))
        {
            Gp retAddr = cc.new_gp64();
            cc.lea(retAddr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_boxed),
                      FuncSignature::build<void, JitContext*, const value::Value*>());
            inv->set_arg(0, s.ctxPtr);
            inv->set_arg(1, retAddr);
        }
        else if (retType == SlotType::FLOAT)
        {
            Vec retVal = cc.new_xmm();
            cc.movsd(retVal, Mem(s.stackBase, s.stackDepth * 8));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_float),
                      FuncSignature::build<void, JitContext*, double>());
            inv->set_arg(0, s.ctxPtr);
            inv->set_arg(1, retVal);
        }
        else if (retType == SlotType::BOOL)
        {
            Gp retVal = cc.new_gp64();
            cc.mov(retVal, Mem(s.stackBase, s.stackDepth * 8));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_bool),
                      FuncSignature::build<void, JitContext*, int64_t>());
            inv->set_arg(0, s.ctxPtr);
            inv->set_arg(1, retVal);
        }
        else
        {
            Gp retVal = cc.new_gp64();
            cc.mov(retVal, Mem(s.stackBase, s.stackDepth * 8));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_int),
                      FuncSignature::build<void, JitContext*, int64_t>());
            inv->set_arg(0, s.ctxPtr);
            inv->set_arg(1, retVal);
        }
    }

    // MYT-259: emit a cc.invoke that pushes the JIT operand-stack top onto
    // the interpreter's operand stack via the appropriate jit_osr_push_*
    // helper. Used by OSR-emitted RETURN_VALUE / CREATE_PROMISE_RETURN_VALUE
    // so the resumed RETURN_VALUE bytecode can pop the value normally.
    // Caller must pass the slotType BEFORE popping s.slotTypes.
    static void emitOsrPushReturnValueToInterpStack(JitEmissionState& s, SlotType retType)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        const int topSlot = s.stackDepth - 1;

        if (isBoxedSlotType(retType))
        {
            // Boxed Value already lives at boxedBase[topSlot]. Pass its
            // address; helper does *val copy into stackManager.
            Gp valAddr = cc.new_gp64();
            cc.lea(valAddr, Mem(s.boxedBase, static_cast<int32_t>(topSlot * valueSize)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_osr_push_value),
                      FuncSignature::build<void, JitContext*, const value::Value*>());
            inv->set_arg(0, s.ctxPtr);
            inv->set_arg(1, valAddr);
        }
        else if (retType == SlotType::FLOAT)
        {
            Vec val = cc.new_xmm();
            cc.movsd(val, Mem(s.stackBase, topSlot * 8));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_osr_push_float),
                      FuncSignature::build<void, JitContext*, double>());
            inv->set_arg(0, s.ctxPtr);
            inv->set_arg(1, val);
        }
        else
        {
            // INT / BOOL — raw int64 in stackBase[topSlot]. Helper picks
            // the right box variant based on which jit_osr_push_* we call.
            Gp val = cc.new_gp64();
            cc.mov(val, Mem(s.stackBase, topSlot * 8));
            const uint64_t fn = (retType == SlotType::BOOL)
                ? reinterpret_cast<uint64_t>(jit_osr_push_bool)
                : reinterpret_cast<uint64_t>(jit_osr_push_int);
            InvokeNode* inv;
            cc.invoke(Out(inv), fn,
                      FuncSignature::build<void, JitContext*, int64_t>());
            inv->set_arg(0, s.ctxPtr);
            inv->set_arg(1, val);
        }
    }

    static void emitReturnValueOp(JitEmissionState& s, const ExitHandler& onExit)
    {
        if (onExit)
        {
            // MYT-185/186/187: snapshot the popped slot type so inline onExit
            // handlers can choose the correct box/unbox direction when
            // materializing the return value at endLabel.
            s.lastReturnSlotType = topType(s);

            // MYT-259: in OSR context (s.isOSRCompilation), the onExit
            // handler is osrExit which resumes the interpreter at a bytecode
            // offset. The previous behaviour passed target=0 meaning
            // "resume after the loop", which silently dropped the function
            // return — the interpreter then fell through to whatever code
            // followed the loop (e.g. the trailing `return -1;` in
            // HashMap.findKeyInBucket). Instead, push the return value to
            // the interpreter's operand stack and resume AT this RETURN_VALUE
            // opcode — the interpreter then runs handleReturnValue() which
            // pops, async-wraps if needed, and does the function-return
            // semantics correctly.
            if (s.isOSRCompilation && s.inlineStack.empty())
            {
                emitOsrPushReturnValueToInterpStack(s, s.lastReturnSlotType);
                s.stackDepth--;
                popType(s);
                onExit(s, s.currentIP);
                return;
            }

            s.stackDepth--;
            popType(s);
            onExit(s, 0);
            return;
        }

        s.stackDepth--;
        SlotType retType = popType(s);
        emitReturnPrimitive(s, retType);
        emitCleanup(s);
        s.cc.ret();
    }

    static std::string resolveCallReturnType(JitEmissionState& s,
                                              const bytecode::BytecodeProgram::Instruction& instr)
    {
        uint32_t nameIndex = static_cast<uint32_t>(instr.operands[0]);
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

    static void emitCallReturnValue(JitEmissionState& s,
                                     const std::string& returnType, bool isPrimReturn)
    {
        // MYT-211: return-value plumbing crosses helper invokes; flush.
        flushAllHints(s);
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        if (isPrimReturn)
        {
            SlotType retSlot = SlotType::INT;
            if (returnType == "float") retSlot = SlotType::FLOAT;
            else if (returnType == "bool") retSlot = SlotType::BOOL;

            Gp retAddr = cc.new_gp64();
            cc.lea(retAddr, Mem(s.ctxPtr, offsetof(JitContext, returnValue)));
            emitUnbox(s, retAddr, s.stackDepth, retSlot);
            s.slotTypes.push_back(retSlot);
        }
        else
        {
            Gp retAddr = cc.new_gp64();
            cc.lea(retAddr, Mem(s.ctxPtr, offsetof(JitContext, returnValue)));
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
            InvokeNode* cpInv;
            cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            cpInv->set_arg(0, destAddr);
            cpInv->set_arg(1, retAddr);
            s.slotTypes.push_back(SlotType::BOXED);
        }
        s.stackDepth++;
    }

    // Phase 1: lower a self-recursive tail call (CALL/CALL_FAST immediately
    // followed by RETURN_VALUE) to an argument-overwrite + jmp to the
    // function's entry label. Eliminates the per-call dispatch plumbing
    // (jit_call_function, JitContext construction, CallFrame push, try/catch)
    // for self-recursive tail calls — e.g. gcd in recursive.mt collapses
    // from 50k recursive invocations into a tight loop.
    //
    // Returns true iff the call site was lowered. False → caller falls
    // through to the generic helper invoke unchanged.
    //
    // Preconditions:
    //   - callee is the currently compiling function (self-recursion)
    //   - next instruction is RETURN_VALUE, and that RETURN_VALUE is not a
    //     jump target (so lowering leaves it as dead-code-after-jmp)
    //   - frame is !usesBoxedTypes and all params are primitive (int/float/
    //     bool); boxed-frame TCO is deferred (requires ref-count-correct
    //     local overwrite via emitInlineLocalCopy)
    //   - argCount matches parameterCount, and the top-of-stack arg
    //     SlotTypes match the declared param types exactly
    static bool tryEmitSelfTailCall(JitEmissionState& s,
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
        // slots so the copy sees the correct values. The cc.jmp at the end
        // also requires memory-coherent state at the back-edge target.
        flushAllHints(s);

        const std::string& calleeKey = calleeMeta->mangledName.empty()
            ? calleeMeta->name : calleeMeta->mangledName;
        if (calleeKey != s.currentCompilingFn &&
            calleeMeta->name != s.currentCompilingFn &&
            calleeMeta->mangledName != s.currentCompilingFn)
            return false;

        // The next IP must be inside the currently compiling function's
        // bytecode range — otherwise we'd be peeking at the following
        // function's first instruction, which is unrelated to this call
        // site's control flow. calleeMeta == the compiling function by
        // the self-recursion check above.
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
        // below (for the dead-code RETURN_VALUE) stays consistent with the
        // function's declared return shape.
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
        auto& cc_ = s.cc;
        Label tailOverflowLabel = cc_.new_label();
        Gp vmReg = cc_.new_gp64();
        cc_.mov(vmReg, Mem(s.ctxPtr, offsetof(JitContext, vm)));
        Gp maxReg = cc_.new_gp64();
        cc_.mov(maxReg, Mem(vmReg, vm::runtime::VirtualMachine::kMaxCallStackSizeOffset));
        cc_.inc(s.tailCallCounter);
        cc_.cmp(s.tailCallCounter, maxReg);
        cc_.jae(tailOverflowLabel);

        // existing GC safepoint + back-edge jmp:
        emitInlineGcSafepoint(s);

        cc.jmp(s.functionEntryLabel);

        // MYT-226: throw path — dead fall-through from the jmp above.
        cc_.bind(tailOverflowLabel);
        InvokeNode* throwInv;
        cc_.invoke(Out(throwInv),
                   reinterpret_cast<uint64_t>(jit_throw_stack_overflow),
                   FuncSignature::build<void, size_t>());
        throwInv->set_arg(0, maxReg);
        // helper does not return; asmjit handles the unwind. No trailing ret/jmp.

        // MYT-207 telemetry: compile-time bump (not runtime). Counts the
        // number of CALL sites lowered to a tail-loop, NOT the number of
        // dynamic iterations the loop runs. Single-threaded VM — non-atomic
        // mirrors compileCount.
        if (s.tailCallsOptimized) (*s.tailCallsOptimized)++;
        return true;
    }

    // MYT-207: lower a NON-tail self-recursive CALL/CALL_FAST to a direct
    // asmjit invoke against the function's own FuncNode->label(). Skips the
    // jit_call_function dispatch overhead — concretely:
    //   - jitCodeCache lookupByIndex (we ARE the lookup target)
    //   - tryJitDispatchResolved's CallFrame push + std::vector grow
    //   - JitContext nestedCtx{} zero-init (~256 bytes / 16 variant defaults)
    //   - try/catch SEH frame setup
    // The dominant residual cost on recursive.mt's fib (~2.76M non-tail
    // recursive calls) is exactly that overhead, since fib's body is tiny.
    //
    // Reuses the same JitContext: args are marshalled into ctx->callArgs by
    // emitBoxCallArgs (mirrors the generic path), then ctx->args is pointed
    // at &ctx->callArgs[0] for the duration of the nested invoke and
    // restored afterwards. ctx->returnValue / ctx->hasReturnValue are
    // populated by the callee's RETURN_VALUE.
    //
    // Stack-overflow protection: inline cmp against
    // VirtualMachine::MAX_JIT_NATIVE_DEPTH; on overflow the emission
    // branches to a fallback path that calls the original generic helper,
    // which itself bails to the interpreter via callFunctionFromJit. Tail-
    // optimized frames do NOT appear in stack traces — same accepted
    // semantic as MYT-210's inliner. See ticket discussion.
    //
    // Preconditions (mirror tryEmitSelfTailCall, sans the RETURN_VALUE
    // peek — this is the NON-tail path):
    //   - selfDirectCallEnabled (false for OSR / boxed frames in v1)
    //   - callee is the currently compiling function (self-recursion)
    //   - argCount matches calleeMeta->parameterCount
    //
    // Returns true iff the call site was lowered. False → caller falls
    // through to the generic helper invoke unchanged.
    static bool tryEmitSelfDirectCall(JitEmissionState& s,
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

        // Marshal args from operand stack → ctx->callArgs[0..argCount-1] and
        // pop the operand-stack slot types. Same helper the generic path
        // uses; both the direct invoke and the fallback consume callArgs.
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
        // unchanged after the recursive call returns. The callee's prologue
        // reads ctx->args once (emitArgumentUnboxing) and copies into its
        // locals; we point at our own callArgs[] for that read.
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

        // Reload vmPtr/depth — caller-saved registers are clobbered by the
        // call; let asmjit allocate fresh virtregs so the dec uses live data.
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

        // ---- Fallback path: depth exceeded, defer to the generic helper ----
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

        // Push the return value back onto the operand stack — same path the
        // generic emit takes.
        if (returnType != "void")
            emitCallReturnValue(s, returnType, /*isPrimReturn=*/true);

        if (s.selfDirectCalls) (*s.selfDirectCalls)++;
        return true;
    }

    static bool emitCallOp(JitEmissionState& s,
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

        // Phase 1: self-recursive tail-call optimization. Checked before
        // the inliner because self-recursion is rejected by the inliner
        // (InlineAnalysis.cpp:141-145 SELF_RECURSIVE), so TCO is the only
        // opportunity to elide the call dispatch for recursive functions.
        // MYT-207 Phase 2: non-tail self-recursive direct invoke against
        // the function's own FuncNode->label() — same rationale (the
        // inliner won't fire for self), targets fib's residual ~2.76M
        // non-tail recursive calls in recursive.mt.
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

    static bool emitCallFastOp(JitEmissionState& s,
                                const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        uint32_t funcIndex = static_cast<uint32_t>(instr.operands[0]);
        size_t argCount = instr.operands[1];

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

    bool emitControlFlowOps(JitEmissionState& s,
                            const bytecode::BytecodeProgram::Instruction& instr,
                            const ExitHandler& onExit)
    {
        auto& cc = s.cc;

        switch (instr.opcode)
        {
            case OpCode::LOAD_LOCAL:
            // MYT-199: the interpreter-side type-quickened variants emit
            // identically to their generic base here — emitLoadLocal /
            // emitStoreLocal already infer slot types from JitEmissionState
            // analysis, and a CACHED-specific JIT emit is a follow-up (same
            // deferral as CALL_METHOD_CACHED at JitCompiler_Objects.cpp:1244).
            case OpCode::LOAD_LOCAL_INT:
            case OpCode::LOAD_LOCAL_FLOAT:
            case OpCode::LOAD_LOCAL_BOOL:
            case OpCode::LOAD_LOCAL_BOXED_INST:
                emitLoadLocal(s, instr.operands[0]);
                return true;

            case OpCode::STORE_LOCAL:
            case OpCode::STORE_LOCAL_INT:
            case OpCode::STORE_LOCAL_FLOAT:
            case OpCode::STORE_LOCAL_BOOL:
            case OpCode::STORE_LOCAL_BOXED_INST:
                emitStoreLocal(s, instr.operands[0]);
                return true;

            case OpCode::LOAD_STORE_LOCAL:
                // MYT-202: compile-time fused LOAD_LOCAL src + STORE_LOCAL dst.
                // De-fuse at JIT time — chained emitLoadLocal + emitStoreLocal
                // produces the same native code as the unfused sequence.
                emitLoadLocal(s, instr.operands[0]);
                emitStoreLocal(s, instr.operands[1]);
                return true;

            case OpCode::LOAD_VAR:
            // MYT-204: CACHED variant treated identically at JIT emit time —
            // jit_load_var still does the name-keyed Environment probe; the
            // CACHED win is interpreter-only. Including the CACHED opcode in
            // this case keeps a hot loop JIT-compilable after the interpreter
            // has promoted some LOAD_VAR sites to LOAD_VAR_CACHED.
            case OpCode::LOAD_VAR_CACHED:
                emitLoadVar(s, static_cast<uint32_t>(instr.operands[0]));
                return true;

            case OpCode::STORE_VAR:
            case OpCode::STORE_VAR_CACHED: // MYT-204
                emitStoreVar(s, static_cast<uint32_t>(instr.operands[0]));
                return true;

            case OpCode::DECLARE_VAR: // MYT-208
                emitDeclareVar(s, instr);
                return true;

            case OpCode::JUMP:
            {
                size_t target = instr.operands[0];
                // MYT-211: jump source must leave memory coherent — both the
                // direct jmp target and the onExit (OSR exit) path read stack
                // memory. flushAllHints writes any dirty register-cached slot
                // back; cheap no-op when the cache is already empty.
                flushAllHints(s);
                if (auto* lbl = findInlineJumpLabel(s, target))
                {
                    cc.jmp(*lbl);
                    return true;
                }
                if (onExit && s.labels.find(target) == s.labels.end())
                {
                    onExit(s, target);
                }
                else
                {
                    cc.jmp(s.labels[target]);
                }
                return true;
            }

            case OpCode::JUMP_IF_FALSE:
            {
                size_t target = instr.operands[0];
                s.stackDepth--;
                popType(s);
                Gp cond = cc.new_gp64();
                cc.mov(cond, Mem(s.stackBase, s.stackDepth * 8));
                cc.test(cond, cond);

                if (auto* lbl = findInlineJumpLabel(s, target))
                {
                    cc.jz(*lbl);
                }
                else if (onExit && s.labels.find(target) == s.labels.end())
                {
                    Label continueLoop = cc.new_label();
                    cc.jnz(continueLoop);
                    onExit(s, target);
                    cc.bind(continueLoop);
                }
                else
                {
                    cc.jz(s.labels[target]);
                }
                return true;
            }

            case OpCode::JUMP_IF_TRUE:
            {
                size_t target = instr.operands[0];
                s.stackDepth--;
                popType(s);
                Gp cond = cc.new_gp64();
                cc.mov(cond, Mem(s.stackBase, s.stackDepth * 8));
                cc.test(cond, cond);

                if (auto* lbl = findInlineJumpLabel(s, target))
                {
                    cc.jnz(*lbl);
                }
                else if (onExit && s.labels.find(target) == s.labels.end())
                {
                    Label continueLoop = cc.new_label();
                    cc.jz(continueLoop);
                    onExit(s, target);
                    cc.bind(continueLoop);
                }
                else
                {
                    cc.jnz(s.labels[target]);
                }
                return true;
            }

            case OpCode::JUMP_IF_FALSE_OR_POP:
            case OpCode::JUMP_IF_TRUE_OR_POP:
            {
                // Short-circuit semantics: on jump-taken the condition value is
                // preserved on the runtime stack (becomes the expression result);
                // on fall-through the value is popped. Peek without decrementing
                // the runtime-side counter before the conditional jump.
                size_t target = instr.operands[0];
                bool jumpOnZero = (instr.opcode == OpCode::JUMP_IF_FALSE_OR_POP);

                // MYT-211: short-circuit needs the value preserved past the
                // jump on the taken path, so we flush the cache (memory must
                // hold the value) before testing. Re-read into a fresh reg
                // afterwards isn't needed — test alone suffices for the
                // branch decision.
                flushAllHints(s);
                Gp cond = cc.new_gp64();
                cc.mov(cond, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                cc.test(cond, cond);

                if (auto* lbl = findInlineJumpLabel(s, target))
                {
                    if (jumpOnZero) cc.jz(*lbl);
                    else            cc.jnz(*lbl);
                }
                else if (onExit && s.labels.find(target) == s.labels.end())
                {
                    Label continueLoop = cc.new_label();
                    if (jumpOnZero) cc.jnz(continueLoop);
                    else            cc.jz(continueLoop);
                    onExit(s, target);
                    cc.bind(continueLoop);
                }
                else
                {
                    if (jumpOnZero) cc.jz(s.labels[target]);
                    else            cc.jnz(s.labels[target]);
                }

                // Fall-through path: pop the condition value. The bytecode
                // compiler guarantees BOOL/INT primitive on top at short-circuit
                // sites (ExpressionCompiler.cpp:63-68, 92-97), so no boxed
                // destroy is needed — mirrors the existing JUMP_IF_FALSE/TRUE
                // primitive-only contract.
                popType(s);
                s.stackDepth--;
                return true;
            }

            case OpCode::JUMP_BACK:
            {
                // MYT-211: the loop back-edge target was bound under a
                // memory-coherent invariant (the codegen loop flushes before
                // every cc.bind), so we must flush before the jmp too — the
                // back-edge target reads stack memory.
                flushAllHints(s);
                emitInlineGcSafepoint(s);
                size_t target = instr.operands[0];
                if (auto* lbl = findInlineJumpLabel(s, target))
                    cc.jmp(*lbl);
                else
                    cc.jmp(s.labels[target]);
                return true;
            }

            case OpCode::RETURN:
            {
                if (onExit)
                {
                    // MYT-259: in OSR, resume at THIS opcode so the
                    // interpreter runs handleReturn() (call-frame pop, IP
                    // restore). Pre-fix used target=0 → resume after the
                    // loop, which dropped the function return entirely.
                    // No value to push (RETURN has no operand stack value).
                    onExit(s, s.isOSRCompilation ? s.currentIP : 0);
                }
                else
                {
                    emitCleanup(s);
                    cc.mov(byte_ptr(s.ctxPtr, offsetof(JitContext, hasReturnValue)), 0);
                    cc.ret();
                }
                return true;
            }

            case OpCode::RETURN_VALUE:
                emitReturnValueOp(s, onExit);
                return true;

            case OpCode::CALL:
                return emitCallOp(s, instr);

            case OpCode::CALL_FAST:
                return emitCallFastOp(s, instr);

            default:
                return false;
        }
    }
}
