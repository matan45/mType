#include "JitCompiler.hpp"
#include <cstdint>
#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "codegen/OSREntryCodegen.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>
#include <unordered_set>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    static void emitWriteBackNonBoxedLocal(Compiler& cc, Gp destAddr,
                                             Gp localsBase, size_t slot, SlotType lt)
    {
        if (lt == SlotType::FLOAT)
        {
            Vec val = cc.new_xmm();
            cc.movsd(val, Mem(localsBase, static_cast<int32_t>(slot * 8)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_box_float),
                      FuncSignature::build<void, value::Value*, double>());
            inv->set_arg(0, destAddr);
            inv->set_arg(1, val);
        }
        else
        {
            Gp val = cc.new_gp64();
            cc.mov(val, Mem(localsBase, static_cast<int32_t>(slot * 8)));
            uint64_t fn = (lt == SlotType::BOOL)
                ? reinterpret_cast<uint64_t>(jit_box_bool)
                : reinterpret_cast<uint64_t>(jit_box_int);
            InvokeNode* inv;
            cc.invoke(Out(inv), fn,
                      FuncSignature::build<void, value::Value*, int64_t>());
            inv->set_arg(0, destAddr);
            inv->set_arg(1, val);
        }
    }

    void emitLocalsWriteBack(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        Gp outPtr = cc.new_gp64();
        cc.mov(outPtr, Mem(s.ctxPtr, offsetof(JitContext, osrOutputLocals)));

        for (size_t i = 0; i < s.localCount; ++i)
        {
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(outPtr, static_cast<int32_t>(i * valueSize)));
            SlotType lt = s.localTypes.count(i) ? s.localTypes[i] : SlotType::INT;

            if (s.usesBoxedTypes)
            {
                Gp srcAddr = cc.new_gp64();
                cc.lea(srcAddr, Mem(s.localsBase, static_cast<int32_t>(i * s.localStride)));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                          FuncSignature::build<void, value::Value*, const value::Value*>());
                inv->set_arg(0, destAddr);
                inv->set_arg(1, srcAddr);
            }
            else
            {
                emitWriteBackNonBoxedLocal(cc, destAddr, s.localsBase, i, lt);
            }
        }
    }

    struct OSRFrame
    {
        Gp localsBase;
        Gp stackBase;
        Gp boxedBase;
        Gp progPtr;
        bool usesBoxedTypes;
        size_t localStride;
        std::unordered_map<size_t, SlotType> localTypes;
    };

    // MYT-211: scan the OSR loop range for slots that are read but never
    // written inside the loop. Hoist a single load of each such slot into a
    // virtreg at OSR-prologue end so emitLoadLocal can short-circuit to the
    // cached register on every iteration. This eliminates the per-iter memory
    // load for loop-invariant locals (e.g. `mask`, `xorKey`, `wordMask`, `N`
    // in the bitwise / arithmetic micro-benchmarks).
    static void hoistInvariantLocals(JitEmissionState& s,
                                      const bytecode::BytecodeProgram& program,
                                      size_t loopStartOffset, size_t loopEndOffset)
    {
        if (s.usesBoxedTypes) return;
        std::unordered_set<size_t> writtenSlots;
        std::unordered_set<size_t> readSlots;
        for (size_t ip = loopStartOffset; ip <= loopEndOffset; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            switch (instr.opcode)
            {
                case OpCode::STORE_LOCAL:
                case OpCode::STORE_LOCAL_INT:
                case OpCode::STORE_LOCAL_FLOAT:
                case OpCode::STORE_LOCAL_BOOL:
                case OpCode::STORE_LOCAL_BOXED_INST:
                case OpCode::ADD_INT_STORE_LOCAL:        // dst slot in operand[0]
                    if (!instr.operands.empty())
                        writtenSlots.insert(instr.operands[0]);
                    break;
                case OpCode::LOAD_STORE_LOCAL:           // src=op[0], dst=op[1]
                    if (instr.operands.size() >= 2)
                    {
                        readSlots.insert(instr.operands[0]);
                        writtenSlots.insert(instr.operands[1]);
                    }
                    break;
                case OpCode::LOAD_LOCAL:
                case OpCode::LOAD_LOCAL_INT:
                case OpCode::LOAD_LOCAL_FLOAT:
                case OpCode::LOAD_LOCAL_BOOL:
                case OpCode::LOAD_LOCAL_BOXED_INST:
                    if (!instr.operands.empty())
                        readSlots.insert(instr.operands[0]);
                    break;
                case OpCode::LOAD_LOAD_ADD_INT:
                case OpCode::LOAD_LOAD_SUB_INT:
                case OpCode::LOAD_LOAD_MUL_INT:
                    if (instr.operands.size() >= 2)
                    {
                        readSlots.insert(instr.operands[0]);
                        readSlots.insert(instr.operands[1]);
                    }
                    break;
                default: break;
            }
        }
        auto& cc = s.cc;
        for (size_t slot : readSlots)
        {
            if (writtenSlots.count(slot)) continue;
            SlotType lt = s.localTypes.count(slot) ? s.localTypes[slot] : SlotType::INT;
            if (isBoxedSlotType(lt)) continue;       // boxed locals stay memory-only
            if (lt == SlotType::FLOAT)
            {
                Vec reg = cc.new_xmm();
                cc.movsd(reg, Mem(s.localsBase, static_cast<int32_t>(slot * 8)));
                s.invariantFloatLocals[slot] = reg;
            }
            else
            {
                Gp reg = cc.new_gp64();
                cc.mov(reg, Mem(s.localsBase, static_cast<int32_t>(slot * 8)));
                s.invariantIntLocals[slot] = reg;
            }
        }
    }

    static OSRFrame setupOSRFrame(Compiler& cc,
                                   const bytecode::BytecodeProgram& program,
                                   const std::vector<LocalSlotInfo>& localSlotInfos,
                                   size_t localCount,
                                   size_t loopStartOffset, size_t loopEndOffset,
                                   Gp ctxPtr)
    {
        // MYT-251: use the single source of truth in JitEmissionState (was
        // a local 64 here that could silently drift from the emitters'
        // bound check). Pair with the operand-stack pre-scan in the inline
        // guards (JitCompiler_Objects.cpp) so a hot OSR loop body that
        // would overflow this budget bails at compile time instead of
        // smashing the C++ /GS cookie at runtime.
        constexpr size_t MAX_OP_STACK = JitEmissionState::MAX_OP_STACK;
        constexpr size_t valueSize = sizeof(value::Value);

        bool usesBoxedTypes = scanOpcodesForBoxedTypes(program, loopStartOffset, loopEndOffset + 1);
        if (!usesBoxedTypes)
        {
            for (const auto& ls : localSlotInfos)
            {
                if (isBoxedSlotType(ls.type))
                {
                    usesBoxedTypes = true;
                    break;
                }
            }
        }

        // MYT-251: also mirror setupCompilationFrame's parameterTypes scan
        // (JitCompiler_Core.cpp:480-490). Find the FunctionMetadata whose
        // bytecode range contains loopStartOffset; if any of its parameter
        // types is a non-primitive (i.e. a value-class or reference type),
        // the boxed lane must be allocated even if neither the loop body
        // nor the captured locals reach it directly. Without this, an
        // inlined value-object receiver path inside the OSR'd loop reads
        // an uninitialized boxedBase. Class methods are registered in the
        // same function table by mangled name (see MethodCompilerHelper.cpp
        // ::registerFunction call sites), so this scan covers them too.
        if (!usesBoxedTypes)
        {
            for (const auto& [name, fm] : program.getFunctions())
            {
                if (fm.instructionCount == 0) continue;
                const size_t fnEnd = fm.startOffset + fm.instructionCount;
                if (loopStartOffset >= fm.startOffset && loopStartOffset < fnEnd)
                {
                    for (const auto& t : fm.parameterTypes)
                    {
                        if (t != "int" && t != "float" && t != "bool")
                        {
                            usesBoxedTypes = true;
                            break;
                        }
                    }
                    break;
                }
            }
        }

        const size_t localStride = usesBoxedTypes ? valueSize : 8;

        // MYT-163: reserve INLINE_LOCALS_SLACK slots so CALL_METHOD sites
        // inside an OSR'd loop body can speculatively inline. Mirrors the
        // equivalent widening in setupCompilationFrame.
        const size_t reservedLocals = localCount + JitEmissionState::INLINE_LOCALS_SLACK;

        Mem localsArea = cc.new_stack(static_cast<uint32_t>(reservedLocals * localStride), 8);
        Gp localsBase = cc.new_gp64("localsBase");
        cc.lea(localsBase, localsArea);

        Mem stackArea = cc.new_stack(MAX_OP_STACK * 8, 8);
        Gp stackBase = cc.new_gp64("stackBase");
        cc.lea(stackBase, stackArea);

        Gp boxedBase = cc.new_gp64("boxedBase");
        if (usesBoxedTypes)
        {
            Mem boxedArea = cc.new_stack(static_cast<uint32_t>(MAX_OP_STACK * valueSize), 8);
            cc.lea(boxedBase, boxedArea);
        }

        Gp progPtr = cc.new_gp64("progPtr");
        if (usesBoxedTypes)
            cc.mov(progPtr, reinterpret_cast<uint64_t>(&program));

        if (usesBoxedTypes)
            emitMemoryInit(cc, localsBase, reservedLocals, boxedBase, MAX_OP_STACK);

        return {localsBase, stackBase, boxedBase, progPtr,
                usesBoxedTypes, localStride, {}};
    }

    static void emitOSRPrologue(Compiler& cc, const OSRFrame& frame, Gp ctxPtr,
                                 const std::vector<LocalSlotInfo>& localSlotInfos,
                                 size_t localCount,
                                 std::unordered_map<size_t, SlotType>& localTypes)
    {
        codegen::OSREntryCodegen::EntryInfo entryInfo;
        entryInfo.localsBase = frame.localsBase;
        entryInfo.stackBase = frame.stackBase;
        entryInfo.boxedBase = frame.boxedBase;
        entryInfo.ctxPtr = ctxPtr;
        entryInfo.progPtr = frame.progPtr;
        entryInfo.localCount = localCount;
        entryInfo.localStride = frame.localStride;
        entryInfo.usesBoxedTypes = frame.usesBoxedTypes;
        codegen::OSREntryCodegen::emitStateLoad(cc, entryInfo, localSlotInfos, localTypes);

        if (!frame.usesBoxedTypes)
        {
            std::unordered_set<size_t> capturedSlots;
            for (const auto& ls : localSlotInfos)
                capturedSlots.insert(ls.slot);
            for (size_t i = 0; i < localCount; ++i)
            {
                if (capturedSlots.find(i) == capturedSlots.end())
                    cc.mov(qword_ptr(frame.localsBase, static_cast<int32_t>(i * 8)), 0);
            }
        }
    }

    static void emitOSRCodegenLoop(JitEmissionState& s,
                                    const ExitHandler& osrExit,
                                    size_t loopStartOffset, size_t loopEndOffset,
                                    const bytecode::BytecodeProgram& program)
    {
        for (size_t ip = loopStartOffset; ip <= loopEndOffset && !s.compileFailed; ++ip)
        {
            auto labelIt = s.labels.find(ip);
            if (labelIt != s.labels.end())
            {
                // MYT-211: see emitCodegenLoop in JitCompiler_Core.cpp.
                flushAllHints(s);
                s.cc.bind(labelIt->second);
                invalidateAllHints(s);
                if (s.backEdgeTargets.find(ip) == s.backEdgeTargets.end())
                    s.arrayInfoCache.clear();

                // MYT-254 (defense-in-depth): at every back-edge target
                // (top of every loop iteration), check whether a JIT helper
                // has stashed an exception on ctx->pendingException and
                // fall into osrExit if so. Without this the helper's own
                // `if (ctx->pendingException) return;` early-out turns
                // every subsequent CALL_METHOD into a silent no-op and the
                // OSR'd loop spins forever instead of surfacing the throw.
                // OSRManager::executeOSRLoop rethrows immediately after the
                // JIT call returns, so the path is: helper catches → loop
                // back-edge sees → osrExit → OSRManager rethrows. Bounded
                // one cc.invoke + cmp + je per outer iteration (back-edges
                // are rare; one per loop level).
                if (s.backEdgeTargets.find(ip) != s.backEdgeTargets.end())
                {
                    InvokeNode* checkInv = nullptr;
                    s.cc.invoke(Out(checkInv),
                                reinterpret_cast<uint64_t>(jit_has_pending_exception),
                                FuncSignature::build<int64_t, const JitContext*>());
                    checkInv->set_arg(0, s.ctxPtr);
                    Gp pendingResult = s.cc.new_gp64();
                    checkInv->set_ret(0, pendingResult);

                    Label noPendingExc = s.cc.new_label();
                    s.cc.test(pendingResult, pendingResult);
                    s.cc.je(noPendingExc);
                    osrExit(s, ip);
                    s.cc.bind(noPendingExc);
                }
            }

            const auto& instr = program.getInstruction(ip);
            s.currentIP = ip;

            uint8_t opByte = static_cast<uint8_t>(instr.opcode);

            if (emitCoreOps(s, instr)) continue;
            if (emitArithmeticOps(s, instr)) continue;
            if (emitControlFlowOps(s, instr, osrExit)) continue;
            emitObjectOps(s, instr);

            // If any of the emitters set compileFailed without attaching a
            // specific reason (e.g. a sub-emitter hit an internal guard),
            // record a generic CODEGEN_FAILURE so the profile still gets
            // a non-NONE reason.
            if (s.compileFailed && s.osrBailoutReason == OSRBailoutReason::NONE)
            {
                s.osrBailoutReason = OSRBailoutReason::CODEGEN_FAILURE;
                s.osrBailoutOpcode = opByte;
            }
        }
    }

    static bool emitOSRBody(Compiler& cc, Gp ctxPtr,
                             const bytecode::BytecodeProgram& program,
                             const OSRFrame& frame,
                             const std::vector<LocalSlotInfo>& localSlotInfos,
                             size_t localCount,
                             size_t loopStartOffset, size_t loopEndOffset,
                             size_t jumpBackOffset,
                             ic::TypeFeedbackCollector* typeFeedback,
                             uint64_t* inlineFieldICHits,
                             uint64_t* inlineFieldICMisses,
                             uint64_t* inlineFieldSetICHits,
                             uint64_t* inlineFieldSetICMisses,
                             InlineDecisionCounters* inlineDecisions,
                             OSRBailoutReason& outReason,
                             uint8_t& outOffendingOpcode)
    {
        std::unordered_map<size_t, SlotType> localTypes;
        emitOSRPrologue(cc, frame, ctxPtr, localSlotInfos, localCount, localTypes);

        auto labels = createJumpLabels(cc, program, loopStartOffset, loopEndOffset + 1,
                                       loopStartOffset, loopEndOffset);
        auto backEdges = collectBackEdgeTargets(program, loopStartOffset, loopEndOffset + 1);
        size_t resumeOffset = loopEndOffset + 1;

        // MYT-211: construct the emission state before the cc.jmp to the loop
        // back-edge target so loop-invariant locals can be hoisted into
        // virtregs first — the hoisted cc.mov instructions must be reached
        // by the runtime before control jumps into the loop body that reads
        // them.
        JitEmissionState s{cc, ctxPtr, frame.localsBase, frame.stackBase,
                           frame.boxedBase, frame.progPtr,
                           frame.usesBoxedTypes, localCount, frame.localStride,
                           0, {}, /*slotHints=*/{}, /*invariantIntLocals=*/{},
                           /*invariantFloatLocals=*/{}, localTypes, false,
                           OSRBailoutReason::NONE, 0,
                           0, labels, program,
                           typeFeedback, {}, backEdges};
        s.inlineFieldICHits = inlineFieldICHits;
        s.inlineFieldICMisses = inlineFieldICMisses;
        s.inlineFieldSetICHits = inlineFieldSetICHits;
        s.inlineFieldSetICMisses = inlineFieldSetICMisses;
        s.inlineDecisions = inlineDecisions;
        // MYT-251: explicit OSR-context signal. Replaces the
        // currentCompilingFn.empty() heuristic at OSR-emit sites
        // (e.g. tryEmitInlinedMethodCall's gate). currentCompilingFn
        // remains empty here as before — it has no meaningful value
        // for an OSR'd loop body — but downstream logic now keys off
        // this flag instead of inferring OSR from the empty string.
        s.isOSRCompilation = true;
        // Phase 1 (self-recursive TCO): OSR frames don't bind functionEntryLabel
        // (the generated code enters at the loop's back-edge target, not a
        // function prologue), so suppress tail-call lowering in OSR emission.
        s.selfTailCallEnabled = false;

        // MYT-211: invariant-local hoisting is disabled in this iteration —
        // suspected source of the hang because asmjit's RA may not preserve
        // virtreg liveness across the prologue→loop-body cc.jmp when the def
        // (cc.mov from local memory) and uses (loop-body cc.cmp/cc.add) are
        // in different basic blocks separated by an unconditional jump that
        // doesn't bind a label. Re-enable after debugging confirms safe.
        // hoistInvariantLocals(s, program, loopStartOffset, loopEndOffset);

        // MYT-153 Bug #2 (double-count): the captured state reflects the
        // interpreter right before the JUMP_BACK at `jumpBackOffset` fires,
        // so resuming at `loopStartOffset` would re-run the block we've
        // already executed (header, inc, or body depending on which
        // back-edge triggered OSR). Jump directly to the back-edge's target
        // — the same instruction the interpreter's JUMP_BACK would have
        // landed on.
        const auto& jbInstr = program.getInstruction(jumpBackOffset);
        if (!jbInstr.operands.empty())
        {
            size_t jumpBackTarget = jbInstr.operands[0];
            auto it = labels.find(jumpBackTarget);
            if (it != labels.end())
                cc.jmp(it->second);
        }
        // MYT-207: same constraint for the direct-self-call path. The OSR
        // entry's FuncNode->label() points at the OSR loop prelude, not the
        // original function's prologue — recursing into it would re-execute
        // the OSR setup with bogus state. currentCompilingFn is left empty
        // for OSR frames anyway, but keep the explicit gate for clarity.
        s.selfDirectCallEnabled = false;

        ExitHandler osrExit = [&](JitEmissionState& es, size_t target) {
            // MYT-211: flush register-cached slots so emitLocalsWriteBack
            // reads consistent memory and any deopt resume sees the correct
            // operand-stack content. emitCleanup also flushes, but doing it
            // here keeps the locals-writeback ordering crystal clear.
            flushAllHints(es);
            emitLocalsWriteBack(es);
            size_t exitTarget = (target == 0) ? resumeOffset : target;
            es.cc.mov(qword_ptr(es.ctxPtr, offsetof(JitContext, osrExitOffset)),
                      static_cast<int64_t>(exitTarget));
            es.cc.mov(byte_ptr(es.ctxPtr, offsetof(JitContext, osrExited)), 1);
            emitCleanup(es);
            es.cc.ret();
        };

        emitOSRCodegenLoop(s, osrExit, loopStartOffset, loopEndOffset, program);

        if (s.compileFailed)
        {
            outReason = (s.osrBailoutReason == OSRBailoutReason::NONE)
                         ? OSRBailoutReason::CODEGEN_FAILURE
                         : s.osrBailoutReason;
            outOffendingOpcode = s.osrBailoutOpcode;
            return false;
        }

        emitCleanup(s);
        cc.mov(byte_ptr(ctxPtr, offsetof(JitContext, hasReturnValue)), 0);
        return true;
    }

    bool JitCompiler::compileLoopOSR(size_t loopStartOffset,
                                      size_t loopEndOffset,
                                      size_t jumpBackOffset,
                                      const std::vector<LocalSlotInfo>& localSlotInfos,
                                      size_t localCount,
                                      const bytecode::BytecodeProgram& program,
                                      JitCodeCache& codeCache,
                                      ic::TypeFeedbackCollector* typeFeedback,
                                      OSRBailoutReason* outReason,
                                      uint8_t* outOffendingOpcode)
    {
        // MYT-148: helper to record bailout reason + opcode for the caller
        // (OSRManager) to attach to the LoopProfile. Single place to update.
        auto reportBailout = [&](OSRBailoutReason reason, uint8_t opcode = 0)
        {
            if (outReason) *outReason = reason;
            if (outOffendingOpcode) *outOffendingOpcode = opcode;
        };

        std::string osrKey = "osr@" + std::to_string(jumpBackOffset);
        if (codeCache.contains(osrKey))
            return true;

        uint8_t offendingOpcode = 0;
        if (!canCompileLoopOSR(loopStartOffset, loopEndOffset, program, &offendingOpcode))
        {
            bailoutCount++;
            reportBailout(OSRBailoutReason::UNSUPPORTED_OPCODE, offendingOpcode);
            return false;
        }

        if (localCount == 0) localCount = 1;
        constexpr size_t MAX_LOCAL_COUNT = 1024;
        if (localCount > MAX_LOCAL_COUNT)
        {
            bailoutCount++;
            reportBailout(OSRBailoutReason::LOCAL_COUNT_EXCEEDED);
            return false;
        }

        CodeHolder code;
        code.init(codeCache.getRuntime().environment());
        Compiler cc(&code);

        FuncNode* func = cc.add_func(FuncSignature::build<void, JitContext*>());
        Gp ctxPtr = cc.new_gp64("ctx");
        func->set_arg(0, ctxPtr);

        auto frame = setupOSRFrame(cc, program, localSlotInfos, localCount,
                                   loopStartOffset, loopEndOffset, ctxPtr);

        OSRBailoutReason bodyReason = OSRBailoutReason::NONE;
        uint8_t bodyOpcode = 0;
        if (!emitOSRBody(cc, ctxPtr, program, frame, localSlotInfos,
                         localCount, loopStartOffset, loopEndOffset,
                         jumpBackOffset, typeFeedback,
                         &inlineFieldICHits, &inlineFieldICMisses,
                         &inlineFieldSetICHits, &inlineFieldSetICMisses,
                         &inlineDecisions,
                         bodyReason, bodyOpcode))
        {
            bailoutCount++;
            reportBailout(bodyReason, bodyOpcode);
            return false;
        }

        cc.end_func();
        return finalizeAndStore(cc, code, codeCache, osrKey,
                               compileCount, bailoutCount);
    }
}
