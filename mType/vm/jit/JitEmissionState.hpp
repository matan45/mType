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
    struct InlineDecisionCounters;
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

    // MYT-211: virtual-register operand-stack cache. The interpreter's JIT
    // historically materialized every push/pop through Mem(stackBase, depth*8),
    // which left the asmjit register allocator no liberty to coalesce adjacent
    // ops — every arithmetic instruction round-tripped through L1. A SlotHint
    // records that a stack slot's current value is also live in a virtreg so
    // the next consumer can skip the memory load. When `dirty == true` the
    // memory at the slot is stale relative to the register; the consumer must
    // either consume from the register or flushSlot(...) before reading
    // memory directly. When `isConstant == true`, the slot holds a known
    // immediate value (PUSH_INT / PUSH_BOOL); shift-range checks and other
    // peephole folds key off this. Hints are only published in the !boxed
    // path (s.usesBoxedTypes == false) — the boxed value-stack pipeline is
    // unchanged. Non-hint-aware emitters call flushAllHints(s) at entry to
    // make memory coherent, so they continue to read Mem(stackBase, ...)
    // exactly as before.
    struct SlotHint
    {
        asmjit::x86::Gp gp;            // Gp virtreg, valid when valid && !isXmm
        asmjit::x86::Vec xmm;          // Xmm virtreg, valid when valid && isXmm
        int64_t constValue = 0;        // populated when isConstant
        bool valid = false;            // true if gp/xmm holds the slot's value
        bool dirty = false;            // memory stale vs register
        bool isXmm = false;            // value is in xmm (FLOAT) rather than gp
        bool isConstant = false;       // constValue holds the slot's value
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
        // MYT-211: per-stack-slot hint cache, sized in lockstep with stackDepth.
        // Only populated when !usesBoxedTypes; in boxed mode left empty so all
        // accesses go through memory exactly as before.
        std::vector<SlotHint> slotHints;
        // MYT-211: read-only locals hoisted to virtregs at OSR-prologue end.
        // emitLoadLocal short-circuits to slotHints with the cached reg when
        // the slot index is present here. STORE_LOCAL to a hoisted slot is
        // a programming error (the slot would not have been hoisted) — guarded
        // by an explicit check at population time.
        std::unordered_map<size_t, asmjit::x86::Gp> invariantIntLocals;
        std::unordered_map<size_t, asmjit::x86::Vec> invariantFloatLocals;
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

        // MYT-191: separate SET-site counters, plumbed the same way.
        uint64_t* inlineFieldSetICHits = nullptr;
        uint64_t* inlineFieldSetICMisses = nullptr;

        // MYT-210 (fills MYT-179 stub): per-decision inline-eligibility
        // counters, bumped at compile time inside tryEmitInlinedMethodCall
        // after checkInlineEligibility returns. Pointer-to-shared instance
        // owned by JitCompiler.
        InlineDecisionCounters* inlineDecisions = nullptr;

        // MYT-163: name of the top-level function currently being compiled.
        // Used by InlineAnalysis::checkInlineEligibility to reject self-recursive
        // inline candidates. Empty for OSR emission (self-recursion already
        // guarded by loop-specific logic).
        std::string currentCompilingFn;

        // MYT-251: explicit OSR-context signal. Replaces the prior
        // `currentCompilingFn.empty()` heuristic — set true by
        // setupOSRFrame's caller (compileLoopOSR) and false (default) for
        // function-level emission via setupCompilationFrame. Sites that
        // need to vary behavior between OSR and function-level should read
        // this directly instead of testing currentCompilingFn for emptiness.
        bool isOSRCompilation = false;

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

        // Self-recursive tail-call optimization: entry label for the function
        // body, bound in emitFunctionBody right before emitCodegenLoop begins.
        // CALL / CALL_FAST emitters detect `return self(...)` shapes and lower
        // them to an argument-overwrite + jmp to this label, collapsing
        // tail recursion into a tight loop (e.g. gcd in recursive.mt). OSR-
        // compiled frames leave this unbound and set selfTailCallEnabled=false
        // so the emitter falls through to the generic helper invoke.
        asmjit::Label functionEntryLabel;
        asmjit::x86::Gp tailCallCounter;  // MYT-226: per-frame depth counter
        bool selfTailCallEnabled = true;

        // MYT-207: self-recursive direct-call optimization for NON-tail sites.
        // selfFuncCallLabel == FuncNode->label() of the currently compiling
        // function — emitted by `cc.invoke(label, ...)` in tryEmitSelfDirectCall
        // to skip jit_call_function's dispatch overhead (CallFrame push,
        // JitContext zero-init, exception-frame setup). Tail-optimized frames
        // do NOT appear in stack traces and recursive depth is not bounded by
        // the CallFrame stack — same semantic for the SelfDirectCall path.
        // OSR frames disable this (their func->label() points at the OSR
        // prelude, not the function prologue).
        asmjit::Label selfFuncCallLabel;
        bool selfDirectCallEnabled = true;

        // MYT-207: counters bumped at compile time inside tryEmitSelfTailCall
        // and tryEmitSelfDirectCall. Pointers owned by JitCompiler; assigned
        // in emitFunctionBody. Default null so OSR / unit tests that don't
        // wire them stay safe via a null guard at the increment site.
        uint64_t* tailCallsOptimized = nullptr;
        uint64_t* selfDirectCalls = nullptr;

        // MYT-251 step 2: constants widened (64 → 256, 32 → 96) alone, with
        // the MYT-248/249/250 workaround (s.currentCompilingFn.empty() bail
        // in tryEmitInlinedMethodCall) still active. While the workaround
        // is in place OSR-loop inlining doesn't fire — these wider budgets
        // only affect frame sizing in setupOSRFrame and
        // setupCompilationFrame. Step 3 plumbs in the peak-scan guards on
        // function-level inlining; step 4 removes the workaround and lets
        // OSR-emitted bodies inline again.
        static constexpr size_t MAX_OP_STACK = 256;
        static constexpr size_t VALUE_SIZE = sizeof(value::Value);

        // MYT-251 step 2: see MAX_OP_STACK comment.
        static constexpr size_t INLINE_LOCALS_SLACK = 96;
    };

    // MYT-251: bounds-check stackDepth bumps in JIT emit. Belt-and-suspenders
    // against the bug class fixed by MYT-248/249/250 — if a future emit path
    // would push past MAX_OP_STACK, mark compileFailed so the JIT bails
    // cleanly instead of emitting code that overflows cc.new_stack at
    // runtime (which silently smashes the C++ /GS cookie via __fastfail).
    // Returns true when there is room, false (and sets compileFailed) when
    // the push would overflow. Callers must respect the false return —
    // typically by skipping the optimization that prompted the push (e.g.
    // bailing an inline candidate).
    inline bool checkOpStackHeadroom(JitEmissionState& s, int additionalPushes = 1)
    {
        if (static_cast<size_t>(s.stackDepth) + additionalPushes
            > JitEmissionState::MAX_OP_STACK)
        {
            s.compileFailed = true;
            return false;
        }
        return true;
    }

    // MYT-211: virtual-register operand-stack helpers. See SlotHint for the
    // model. All functions are no-ops in boxed mode (s.usesBoxedTypes == true)
    // so the boxed pipeline keeps its memory-only behavior unchanged.
    void publishGpHint(JitEmissionState& s, int stackIdx,
                       asmjit::x86::Gp reg);
    void publishXmmHint(JitEmissionState& s, int stackIdx,
                        asmjit::x86::Vec reg, bool dirty);
    // Record-only variants for emitters that have already written stackBase
    // memory themselves (e.g. emitUnbox in the boxed-mode LOAD_LOCAL path).
    // These ONLY update the hint cache — they don't emit any cc.* instruction.
    void recordGpHint(JitEmissionState& s, int stackIdx, asmjit::x86::Gp reg);
    void recordXmmHint(JitEmissionState& s, int stackIdx, asmjit::x86::Vec reg);
    void publishConstHint(JitEmissionState& s, int stackIdx, int64_t value);
    // consumeGpHint: returns a Gp virtreg holding the slot value. If a hint
    // is published, reuses its register (or materializes a constant). Else
    // emits cc.mov(reg, Mem(stackBase, idx*8)). Always clears the slot's
    // hint after the call so subsequent writes don't believe the slot is
    // still cached.
    asmjit::x86::Gp consumeGpHint(JitEmissionState& s, int stackIdx);
    asmjit::x86::Vec consumeXmmHint(JitEmissionState& s, int stackIdx);
    void flushSlot(JitEmissionState& s, int stackIdx);
    void flushAllHints(JitEmissionState& s);
    void invalidateAllHints(JitEmissionState& s);
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
