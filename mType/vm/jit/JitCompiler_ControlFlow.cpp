#include "JitCompiler_ControlFlow.hpp"
#include <cstddef>
#include <cstdint>
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include "../../gc/GCConfig.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    void emitInlineGcSafepoint(JitEmissionState& s)
    {
        // Inline GC poll: bump g_jit_gc_poll_counter and only invoke
        // jit_gc_safepoint (which resets the counter and calls
        // GC::maybeCollect) when it crosses GC_CHECK_INTERVAL. Skips the
        // ABI register-spill overhead of cc.invoke on the common path —
        // critical for tight loops where JUMP_BACK fires millions of times.
        auto& cc = s.cc;
        Gp cntAddr = cc.new_gp64();
        Gp cnt = cc.new_gp64();
        cc.mov(cntAddr, reinterpret_cast<uint64_t>(&g_jit_gc_poll_counter));
        cc.mov(cnt, qword_ptr(cntAddr));
        cc.inc(cnt);
        cc.mov(qword_ptr(cntAddr), cnt);
        cc.cmp(cnt, static_cast<int32_t>(gc::config::GC_CHECK_INTERVAL));
        Label skipLabel = cc.new_label();
        cc.jb(skipLabel);
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_gc_safepoint),
                  FuncSignature::build<void>());
        cc.bind(skipLabel);
    }

    namespace
    {
        // MYT-164 (Phase F-b): resolve a jump target against the innermost
        // active inline frame's localJumpLabels. Returns nullptr when no
        // inline frame is active or the target is outside the callee's
        // bytecode range (in which case the JUMP-family emitter falls back
        // to the outer s.labels / onExit path). Only the top frame is
        // consulted — jumps inside an inner callee cannot target an outer
        // frame's bytecode range by lexical scope.
        Label* findInlineJumpLabel(JitEmissionState& s, size_t target)
        {
            if (s.inlineStack.empty()) return nullptr;
            auto& frame = s.inlineStack.back();
            auto it = frame.localJumpLabels.find(target);
            if (it == frame.localJumpLabels.end()) return nullptr;
            return &it->second;
        }

        bool typedLocalSlotType(OpCode opcode, SlotType& out)
        {
            switch (opcode)
            {
                case OpCode::LOAD_LOCAL_INT:
                case OpCode::STORE_LOCAL_INT:
                    out = SlotType::INT;
                    return true;
                case OpCode::LOAD_LOCAL_FLOAT:
                case OpCode::STORE_LOCAL_FLOAT:
                    out = SlotType::FLOAT;
                    return true;
                case OpCode::LOAD_LOCAL_BOOL:
                case OpCode::STORE_LOCAL_BOOL:
                    out = SlotType::BOOL;
                    return true;
                default:
                    return false;
            }
        }
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
            // analysis, and a CACHED-specific JIT emit is a follow-up.
            case OpCode::LOAD_LOCAL_INT:
            case OpCode::LOAD_LOCAL_FLOAT:
            case OpCode::LOAD_LOCAL_BOOL:
            case OpCode::LOAD_LOCAL_BOXED_INST:
            {
                SlotType forcedType = SlotType::INT;
                bool hasForcedType = typedLocalSlotType(instr.opcode, forcedType);
                emitLoadLocal(s, instr.inlineOperands[0], hasForcedType, forcedType);
                return true;
            }

            case OpCode::STORE_LOCAL:
            case OpCode::STORE_LOCAL_INT:
            case OpCode::STORE_LOCAL_FLOAT:
            case OpCode::STORE_LOCAL_BOOL:
            case OpCode::STORE_LOCAL_BOXED_INST:
            {
                SlotType forcedType = SlotType::INT;
                bool hasForcedType = typedLocalSlotType(instr.opcode, forcedType);
                emitStoreLocal(s, instr.inlineOperands[0], hasForcedType, forcedType);
                return true;
            }

            case OpCode::LOAD_STORE_LOCAL:
                // MYT-202: compile-time fused LOAD_LOCAL src + STORE_LOCAL dst.
                // De-fuse at JIT time — chained emitLoadLocal + emitStoreLocal
                // produces the same native code as the unfused sequence.
                emitLoadLocal(s, instr.inlineOperands[0]);
                emitStoreLocal(s, instr.inlineOperands[1]);
                return true;

            case OpCode::LOAD_VAR:
            // MYT-204: CACHED variant treated identically at JIT emit time —
            // jit_load_var still does the name-keyed Environment probe; the
            // CACHED win is interpreter-only. Including the CACHED opcode in
            // this case keeps a hot loop JIT-compilable after the interpreter
            // has promoted some LOAD_VAR sites to LOAD_VAR_CACHED.
            case OpCode::LOAD_VAR_CACHED:
                emitLoadVar(s, static_cast<uint32_t>(instr.inlineOperands[0]));
                return true;

            case OpCode::STORE_VAR:
            case OpCode::STORE_VAR_CACHED: // MYT-204
                emitStoreVar(s, static_cast<uint32_t>(instr.inlineOperands[0]));
                return true;

            case OpCode::DECLARE_VAR: // MYT-208
                emitDeclareVar(s, instr);
                return true;

            case OpCode::JUMP:
            {
                size_t target = instr.inlineOperands[0];
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
                size_t target = instr.inlineOperands[0];
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
                size_t target = instr.inlineOperands[0];
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
                // Short-circuit semantics: on jump-taken the condition value
                // is preserved on the runtime stack (becomes the expression
                // result); on fall-through the value is popped. Peek without
                // decrementing the runtime-side counter before the
                // conditional jump.
                size_t target = instr.inlineOperands[0];
                bool jumpOnZero = (instr.opcode == OpCode::JUMP_IF_FALSE_OR_POP);

                // MYT-211: short-circuit needs the value preserved past the
                // jump on the taken path, so we flush the cache (memory must
                // hold the value) before testing.
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
                // compiler guarantees BOOL/INT primitive on top at short-
                // circuit sites (ExpressionCompiler.cpp:63-68, 92-97), so no
                // boxed destroy is needed — mirrors the existing JUMP_IF_
                // FALSE/TRUE primitive-only contract.
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
                size_t target = instr.inlineOperands[0];
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
