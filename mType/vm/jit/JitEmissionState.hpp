#pragma once
#include "SlotType.hpp"
#include "JitContext.hpp"
#include "JitHelpers.hpp"
#include "LoopProfiler.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace vm::jit
{
    class JitCodeCache;
    namespace ic { class TypeFeedbackCollector; }
    enum class CmpOp { EQ, NE, LT, GT, LE, GE };

    // MYT-163 (Phase F-a): one InlineFrame per active inlined call site on the
    // current emission path. F-a only ever pushes depth 1 (straight-line MONO
    // callees); the vector is kept so F-b can stack nested inlines without
    // further state-shape churn.
    struct InlineFrame
    {
        const bytecode::BytecodeProgram::FunctionMetadata* calleeMeta = nullptr;
        // First local slot, in the caller's locals[] layout, reserved for the
        // inlined callee's locals. The callee's slot N maps to caller slot
        // (N + localsBaseSlot).
        size_t localsBaseSlot = 0;
        // Where caller's emission resumes after the inlined body runs. Bound
        // at emit time by tryEmitInlinedMethodCall.
        asmjit::Label inlineEndLabel;
        // Runtime stack slot where the caller expects the inlined call's
        // return value to land (boxed operand stack index in both modes).
        int returnResultStackIdx = -1;
        // MYT-164 (Phase F-b): asmjit labels keyed by callee-local target IP,
        // populated by tryEmitInlinedMethodCall's pre-scan over the callee's
        // bytecode range. JUMP-family emitters in JitCompiler_ControlFlow.cpp
        // consult the top-of-stack frame's map before falling back to the
        // outer function's s.labels / onExit path.
        std::unordered_map<size_t, asmjit::Label> localJumpLabels;
    };

    struct JitEmissionState
    {
        asmjit::x86::Compiler& cc;

        asmjit::x86::Gp ctxPtr;
        asmjit::x86::Gp localsBase;
        asmjit::x86::Gp stackBase;
        asmjit::x86::Gp boxedBase;
        asmjit::x86::Gp progPtr;

        bool usesBoxedTypes;
        size_t localCount;
        size_t localStride;

        int stackDepth = 0;
        std::vector<SlotType> slotTypes;
        std::unordered_map<size_t, SlotType> localTypes;
        bool compileFailed = false;
        // MYT-148: when compileFailed is set during OSR emission, record which
        // gate tripped so compileLoopOSR can mark the loop profile with a
        // diagnosable reason. Ignored by function-level compilation.
        OSRBailoutReason osrBailoutReason = OSRBailoutReason::NONE;
        uint8_t osrBailoutOpcode = 0;
        size_t currentIP = 0;
        std::unordered_map<size_t, asmjit::Label> labels;

        const bytecode::BytecodeProgram& program;

        ic::TypeFeedbackCollector* typeFeedback = nullptr;

        struct CachedArrayInfo {
            asmjit::x86::Gp dataPtr;
            asmjit::x86::Gp length;
        };
        std::unordered_map<int, CachedArrayInfo> arrayInfoCache;
        std::unordered_set<size_t> backEdgeTargets;

        // MYT-172 AC #3: counters bumped from emitted code on the inline
        // field-IC fast path (hits) and slow path (misses). Pointers are
        // baked as immediates by tryEmitInlinedFieldGet; the counters
        // themselves live on JitCompiler. Placed after backEdgeTargets so
        // the existing positional brace-init in JitCompiler_Core/_OSR.cpp
        // stays untouched (these get default-initialized to nullptr and
        // are then assigned via s.inlineFieldICHits = &this->...).
        uint64_t* inlineFieldICHits = nullptr;
        uint64_t* inlineFieldICMisses = nullptr;

        // MYT-163: name of the top-level function currently being compiled.
        // Used by InlineAnalysis::checkInlineEligibility to reject self-recursive
        // inline candidates. Empty for OSR emission (self-recursion already
        // guarded by loop-specific logic).
        std::string currentCompilingFn;

        // MYT-163: active inline frames on the current emission path. Size 0
        // when emitting top-level caller code. LOAD_LOCAL / STORE_LOCAL consult
        // inlineLocalsBase (mirroring inlineStack.back().localsBaseSlot during
        // an active inline) to remap the callee's local indices.
        std::vector<InlineFrame> inlineStack;
        size_t inlineLocalsBase = 0;

        // MYT-185/186/187: snapshot of the popped slotTypes top at every
        // RETURN_VALUE emission, set BEFORE emitReturnValueOp calls popType,
        // so inline onExit handlers can tell whether the return value lives
        // in boxedBase (isBoxedSlotType true) or stackBase (INT/FLOAT/BOOL).
        // The materialize helper boxes in one direction or mirrors in the
        // other so the fast-path physical state matches the slow path's
        // emitReturnValueCopyBoxed at endLabel.
        SlotType lastReturnSlotType = SlotType::BOXED;

        static constexpr size_t MAX_OP_STACK = 64;
        static constexpr size_t VALUE_SIZE = sizeof(value::Value);

        // MYT-163: inline-locals slack reserved in every compiled frame so a
        // hot call site can commit an inlined callee's locals without
        // re-allocating stack. Sized conservatively for F-a; F-b will replace
        // with a per-function pre-scan (computeInlineLocalsBudget).
        static constexpr size_t INLINE_LOCALS_SLACK = 32;
    };

    void emitBox(JitEmissionState& s, asmjit::x86::Gp destAddr,
                 int stackOff, SlotType type);
    void emitUnbox(JitEmissionState& s, asmjit::x86::Gp srcAddr,
                   int stackOff, SlotType type);
    void emitEnsureUnboxed(JitEmissionState& s, int stackIdx,
                           SlotType type, SlotType targetType);
    void emitBoxOrCopy(JitEmissionState& s, asmjit::x86::Gp destAddr,
                       int stackOff, SlotType type);
    SlotType popType(JitEmissionState& s);
    SlotType topType(const JitEmissionState& s);
    void emitCleanup(JitEmissionState& s);
    void emitGenericBinop(JitEmissionState& s, uint64_t helperFn,
                          SlotType lType, SlotType rType);
    void emitCmp(JitEmissionState& s, CmpOp kind);

    bool emitCoreOps(JitEmissionState& s,
                     const bytecode::BytecodeProgram::Instruction& instr);

    bool emitArithmeticOps(JitEmissionState& s,
                           const bytecode::BytecodeProgram::Instruction& instr);

    using ExitHandler = std::function<void(JitEmissionState& s, size_t target)>;
    bool emitControlFlowOps(JitEmissionState& s,
                            const bytecode::BytecodeProgram::Instruction& instr,
                            const ExitHandler& onExit);

    bool emitArrayOps(JitEmissionState& s,
                      const bytecode::BytecodeProgram::Instruction& instr);

    bool emitObjectOps(JitEmissionState& s,
                       const bytecode::BytecodeProgram::Instruction& instr);

    void emitValueDestroy(JitEmissionState& s, int slotOffset);
    void emitReturnValueCopyBoxed(JitEmissionState& s);
    asmjit::x86::Gp emitGetBoxedValueAddr(JitEmissionState& s, int stackIdx, SlotType valType);

    // MYT-163 Phase F-a inline-emission helpers.
    void emitInlineShapeGuard(JitEmissionState& s, int receiverStackIdx,
                              const void* expectedShape, asmjit::Label slowLabel);
    void emitInlineLocalCopy(JitEmissionState& s, int receiverStackIdx,
                             size_t localsBaseSlot,
                             const bytecode::BytecodeProgram::FunctionMetadata& callee);

    // MYT-169 (Phase F-a follow-up) Fix B: destroy the boxed inline-local
    // slots that emitInlineLocalCopy aliased via raw memcpy. Emitted at each
    // inline body's exit so the donated shared_ptr ownership is correctly
    // released and the slot is reset to monostate before the next iteration
    // (or the final emitCleanup) runs.
    void emitInlineLocalDestroy(JitEmissionState& s, size_t localsBaseSlot,
                                const bytecode::BytecodeProgram::FunctionMetadata& callee);

    // MYT-185/186/187: at every inline fast-path RETURN_VALUE, converge the
    // runtime physical state with the slow path's emitReturnValueCopyBoxed
    // output so the join at endLabel is consistent regardless of which path
    // executed. Dispatch by the snapshotted return SlotType:
    //   - INT/BOOL/FLOAT: value is in stackBase (LOAD_LOCAL/PUSH_INT/ADD_INT
    //     etc. in boxed-mode emission leave primitives raw in stackBase);
    //     box into boxedBase so downstream boxed consumers get a valid Value.
    //   - BOXED family: value is in boxedBase; mirror to stackBase via
    //     jit_unbox_int (returns 0 for non-numeric variants — harmless, and
    //     matches what the slow path writes for string/object returns).
    void emitInlineReturnMaterialize(JitEmissionState& s, int receiverStackIdx,
                                     SlotType returnSlotType);

    // MYT-165 Phase F-c inline-emission helpers for POLY guard chains.
    asmjit::x86::Gp emitExtractReceiverClassDef(JitEmissionState& s,
                                                 int receiverStackIdx);
    void emitInlineShapeGuardReusingClassDef(JitEmissionState& s,
                                              asmjit::x86::Gp classDefReg,
                                              const void* expectedShape,
                                              asmjit::Label missLabel);

    void emitBoxCallArgs(JitEmissionState& s, size_t argCount,
                         size_t destStartSlot = 0);
    void emitPopAndDestroyArgs(JitEmissionState& s, size_t argCount);

    void emitLocalsWriteBack(JitEmissionState& s);

    bool scanOpcodesForBoxedTypes(const bytecode::BytecodeProgram& program,
                                  size_t startOffset, size_t endOffset);

    std::unordered_map<size_t, asmjit::Label> createJumpLabels(
        asmjit::x86::Compiler& cc, const bytecode::BytecodeProgram& program,
        size_t startOffset, size_t endOffset,
        size_t rangeStart = 0, size_t rangeEnd = SIZE_MAX);

    std::unordered_set<size_t> collectBackEdgeTargets(
        const bytecode::BytecodeProgram& program,
        size_t startOffset, size_t endOffset);

    void emitMemoryInit(asmjit::x86::Compiler& cc,
                        asmjit::x86::Gp localsBase, size_t localCount,
                        asmjit::x86::Gp boxedBase, size_t boxedCount);

    bool finalizeAndStore(asmjit::x86::Compiler& cc, asmjit::CodeHolder& code,
                          JitCodeCache& codeCache, const std::string& key,
                          size_t& compileCount, size_t& bailoutCount);
}
