#include "JitCompiler.hpp"
#include "JitCompiler_OSR_Internal.hpp"
#include <asmjit/x86.h>
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "codegen/OSREntryCodegen.hpp"
#include "../bytecode/OpCode.hpp"

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

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

    static OSRFrame setupOSRFrame(Compiler& cc,
                                   const bytecode::BytecodeProgram& program,
                                   const std::vector<LocalSlotInfo>& localSlotInfos,
                                   size_t localCount,
                                   size_t loopStartOffset, size_t loopEndOffset,
                                   Gp ctxPtr)
    {
        // MYT-251: use the single source of truth in JitEmissionState (was
        // a local 64 here that could silently drift from the emitters' bound
        // check). Pair with the operand-stack pre-scan in the inline guards
        // (JitCompiler_Objects.cpp) so a hot OSR loop body that would
        // overflow cc.new_stack bails at compile time. The MYT-184 /GS cookie
        // symptom that originally motivated reframing this guard turned out
        // to be MYT-321's unary-INT-on-boxed-slot bug; pure operand-stack
        // overflow is still a real cc.new_stack overrun failure mode this
        // budget continues to defend against.
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

                // MYT-254 (defense-in-depth): at every back-edge target (top
                // of every loop iteration), check whether a JIT helper has
                // stashed an exception on ctx->pendingException and fall into
                // osrExit if so. Without this the helper's own `if
                // (ctx->pendingException) return;` early-out turns every
                // subsequent CALL_METHOD into a silent no-op and the OSR'd
                // loop spins forever instead of surfacing the throw.
                // OSRManager::executeOSRLoop rethrows immediately after the
                // JIT call returns, so the path is: helper catches → loop
                // back-edge sees → osrExit → OSRManager rethrows. Bounded
                // one cc.invoke + cmp + je per outer iteration.
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
            // specific reason, record a generic CODEGEN_FAILURE so the
            // profile still gets a non-NONE reason.
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
                             JitCodeCache* codeCache,
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
        s.codeCache = codeCache;
        // MYT-251: explicit OSR-context signal. Replaces the
        // currentCompilingFn.empty() heuristic at OSR-emit sites (e.g.
        // tryEmitInlinedMethodCall's gate). currentCompilingFn remains empty
        // here as before — it has no meaningful value for an OSR'd loop body
        // — but downstream logic now keys off this flag instead of inferring
        // OSR from the empty string.
        s.isOSRCompilation = true;
        // Phase 1 (self-recursive TCO): OSR frames don't bind
        // functionEntryLabel (the generated code enters at the loop's back-
        // edge target, not a function prologue), so suppress tail-call
        // lowering in OSR emission.
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
        // already executed (header, inc, or body depending on which back-
        // edge triggered OSR). Jump directly to the back-edge's target — the
        // same instruction the interpreter's JUMP_BACK would have landed on.
        const auto& jbInstr = program.getInstruction(jumpBackOffset);
        if (jbInstr.hasOperands())
        {
            size_t jumpBackTarget = jbInstr.inlineOperands[0];
            auto it = labels.find(jumpBackTarget);
            if (it != labels.end())
                cc.jmp(it->second);
        }
        // MYT-207: same constraint for the direct-self-call path. The OSR
        // entry's FuncNode->label() points at the OSR loop prelude, not the
        // original function's prologue — recursing into it would re-execute
        // the OSR setup with bogus state.
        s.selfDirectCallEnabled = false;

        ExitHandler osrExit = [&](JitEmissionState& es, size_t target) {
            // MYT-211: flush register-cached slots so emitLocalsWriteBack
            // reads consistent memory and any deopt resume sees the correct
            // operand-stack content.
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
        // (OSRManager) to attach to the LoopProfile.
        auto reportBailout = [&](OSRBailoutReason reason, uint8_t opcode = 0)
        {
            if (outReason) *outReason = reason;
            if (outOffendingOpcode) *outOffendingOpcode = opcode;
        };

        std::string osrKey = "osr@" + std::to_string(jumpBackOffset);
        if (codeCache.contains(osrKey))
            return true;

        OpCode offendingOpcode{};
        if (!canCompileLoopOSR(loopStartOffset, loopEndOffset, program, &offendingOpcode))
        {
            bailoutCount++;
            osrBailoutOpcodes[static_cast<uint8_t>(offendingOpcode)]++;
            reportBailout(OSRBailoutReason::UNSUPPORTED_OPCODE,
                          static_cast<uint8_t>(offendingOpcode));
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
                         &codeCache,
                         &inlineFieldICHits, &inlineFieldICMisses,
                         &inlineFieldSetICHits, &inlineFieldSetICMisses,
                         &inlineDecisions,
                         bodyReason, bodyOpcode))
        {
            bailoutCount++;
            if (bodyOpcode != 0)
                osrBailoutOpcodes[bodyOpcode]++;
            reportBailout(bodyReason, bodyOpcode);
            return false;
        }

        cc.end_func();
        return finalizeAndStore(cc, code, codeCache, osrKey,
                               compileCount, bailoutCount);
    }
}
