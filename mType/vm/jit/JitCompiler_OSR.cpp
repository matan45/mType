#include "JitCompiler.hpp"
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

    static OSRFrame setupOSRFrame(Compiler& cc,
                                   const bytecode::BytecodeProgram& program,
                                   const std::vector<LocalSlotInfo>& localSlotInfos,
                                   size_t localCount,
                                   size_t loopStartOffset, size_t loopEndOffset,
                                   Gp ctxPtr)
    {
        constexpr size_t MAX_OP_STACK = 64;
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
                s.cc.bind(labelIt->second);
                if (s.backEdgeTargets.find(ip) == s.backEdgeTargets.end())
                    s.arrayInfoCache.clear();
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

        JitEmissionState s{cc, ctxPtr, frame.localsBase, frame.stackBase,
                           frame.boxedBase, frame.progPtr,
                           frame.usesBoxedTypes, localCount, frame.localStride,
                           0, {}, localTypes, false,
                           OSRBailoutReason::NONE, 0,
                           0, labels, program,
                           typeFeedback, {}, backEdges};
        s.inlineFieldICHits = inlineFieldICHits;
        s.inlineFieldICMisses = inlineFieldICMisses;
        s.inlineFieldSetICHits = inlineFieldSetICHits;
        s.inlineFieldSetICMisses = inlineFieldSetICMisses;
        s.inlineDecisions = inlineDecisions;
        // Phase 1 (self-recursive TCO): OSR frames don't bind functionEntryLabel
        // (the generated code enters at the loop's back-edge target, not a
        // function prologue), so suppress tail-call lowering in OSR emission.
        s.selfTailCallEnabled = false;

        ExitHandler osrExit = [&](JitEmissionState& es, size_t target) {
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
