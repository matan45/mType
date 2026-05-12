#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "JitCodeCache.hpp"
#include "JitContext.hpp"
#include "SlotType.hpp"
#include "OSRState.hpp"
#include "LoopProfiler.hpp"
#include "../optimization/InlineAnalysis.hpp"
namespace vm::jit::ic { class TypeFeedbackCollector; }
#include "../bytecode/BytecodeProgram.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    // MYT-210 (fills MYT-179 stub): per-decision inline telemetry. Counters
    // are bumped at compile time inside tryEmitInlinedMethodCall and printed
    // by --jit-stats. The array index is the underlying ordinal of
    // optimization::InlineDecision. (Plain-function inliner is currently
    // disabled — perReasonFunction stays all zeros until that path returns;
    // see deferred work in JitCompiler_ControlFlow.cpp emitCallOp/
    // emitCallFastOp.)
    struct InlineDecisionCounters
    {
        // Sized to match the InlineDecision enum. The enum currently has 17
        // values (INLINE..HAS_UNSUPPORTED_OPCODE); kept loose at 32 so adding
        // a new reason in InlineAnalysis.hpp doesn't break this layout.
        static constexpr size_t SIZE = 32;
        std::array<uint64_t, SIZE> perReasonMethod   = {};
        std::array<uint64_t, SIZE> perReasonFunction = {};

        void bumpMethod(optimization::InlineDecision d) {
            const auto idx = static_cast<size_t>(d);
            if (idx < SIZE) perReasonMethod[idx]++;
        }
        void bumpFunction(optimization::InlineDecision d) {
            const auto idx = static_cast<size_t>(d);
            if (idx < SIZE) perReasonFunction[idx]++;
        }
    };

    class JitCompiler
    {
    public:
        JitCompiler();

        bool compile(const std::string& functionName,
                     const bytecode::BytecodeProgram& program,
                     JitCodeCache& codeCache,
                     ic::TypeFeedbackCollector* typeFeedback = nullptr);

        size_t getCompileCount() const { return compileCount; }
        size_t getBailoutCount() const { return bailoutCount; }

        // MYT-172 AC #3: inline field-IC hit/miss counters bumped from JIT
        // emitted code. Pointers are baked as immediates at emit time, so
        // these members must live as long as any compiled function — which
        // they do, since JitCompiler is owned by the VirtualMachine.
        uint64_t getInlineFieldICHits() const { return inlineFieldICHits; }
        uint64_t getInlineFieldICMisses() const { return inlineFieldICMisses; }
        uint64_t* inlineFieldICHitsPtr() { return &inlineFieldICHits; }
        uint64_t* inlineFieldICMissesPtr() { return &inlineFieldICMisses; }

        // MYT-191: separate SET counters (GET misses are dominated by non-
        // primitive tags, SET misses by ValueObject CoW — merging hides
        // regressions).
        uint64_t getInlineFieldSetICHits() const { return inlineFieldSetICHits; }
        uint64_t getInlineFieldSetICMisses() const { return inlineFieldSetICMisses; }
        uint64_t* inlineFieldSetICHitsPtr() { return &inlineFieldSetICHits; }
        uint64_t* inlineFieldSetICMissesPtr() { return &inlineFieldSetICMisses; }

        // MYT-210: per-InlineDecision counters for both method-call and plain
        // function inlining. Read by VirtualMachine::printJitStats.
        const InlineDecisionCounters& getInlineDecisions() const { return inlineDecisions; }
        InlineDecisionCounters* inlineDecisionsPtr() { return &inlineDecisions; }

        // MYT-207: self-recursion call-dispatch telemetry.
        //   tailCallsOptimized — bumped inside tryEmitSelfTailCall on every
        //     `return self(...)` site lowered to arg-overwrite + jmp.
        //   selfDirectCalls    — bumped inside tryEmitSelfDirectCall on every
        //     non-tail self-recursive site lowered to a direct asmjit invoke
        //     against funcNode->label() (skips jit_call_function dispatch).
        // Pointers handed to JitEmissionState so emitted code can post-increment.
        uint64_t getTailCallsOptimized() const { return tailCallsOptimized; }
        uint64_t getSelfDirectCalls()    const { return selfDirectCalls; }
        uint64_t* tailCallsOptimizedPtr() { return &tailCallsOptimized; }
        uint64_t* selfDirectCallsPtr()    { return &selfDirectCalls; }

        // MYT-148: extra out-parameters so the caller (OSRManager) can record
        // WHICH gate rejected the loop in the LoopProfile for --jit-stats.
        // Defaults preserve pre-MYT-148 behavior for any other call site.
        bool compileLoopOSR(size_t loopStartOffset,
                            size_t loopEndOffset,
                            size_t jumpBackOffset,
                            const std::vector<LocalSlotInfo>& localSlotInfos,
                            size_t localCount,
                            const bytecode::BytecodeProgram& program,
                            JitCodeCache& codeCache,
                            ic::TypeFeedbackCollector* typeFeedback = nullptr,
                            OSRBailoutReason* outReason = nullptr,
                            uint8_t* outOffendingOpcode = nullptr);

    private:
        bool canCompile(const bytecode::BytecodeProgram::FunctionMetadata& meta,
                        const bytecode::BytecodeProgram& program) const;

        // Returns true if every opcode in [start, end] is JIT-supported. On
        // failure, *outOpcode (if non-null) gets the offending opcode.
        bool canCompileLoopOSR(size_t loopStartOffset, size_t loopEndOffset,
                               const bytecode::BytecodeProgram& program,
                               bytecode::OpCode* outOpcode = nullptr) const;

        static const std::unordered_set<uint8_t>& getSupportedOpcodes();

        // MYT-302 follow-up: counts how many times the for-loop-condition
        // safety guard in hasForwardConditionalCallRegion fired during
        // canCompileLoopOSR scans. The guard pattern-matches the bytecode
        // compiler's lowering of `for` condition exits (JUMP_IF_FALSE+JUMP);
        // if the compiler ever changes that lowering the guard silently
        // becomes a no-op. Tests / --jit-stats can read this counter to
        // detect regressions where the guard stops firing.
        static uint64_t getLoopConditionGuardHits();

        size_t compileCount = 0;
        size_t bailoutCount = 0;
        uint64_t inlineFieldICHits = 0;
        uint64_t inlineFieldICMisses = 0;
        uint64_t inlineFieldSetICHits = 0;
        uint64_t inlineFieldSetICMisses = 0;
        InlineDecisionCounters inlineDecisions;
        // MYT-207 telemetry — see public getters for description.
        uint64_t tailCallsOptimized = 0;
        uint64_t selfDirectCalls = 0;
    };
}
