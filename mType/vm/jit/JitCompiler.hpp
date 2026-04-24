#pragma once
#include <array>
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
    // MYT-210 (fills MYT-179 stub): per-decision inline telemetry. Counters are
    // bumped at compile time inside tryEmitInlinedMethodCall /
    // tryEmitInlinedFunctionCall and printed by --jit-stats. The array index
    // is the underlying ordinal of optimization::InlineDecision.
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
        // failure, *outOpcode (if non-null) gets the offending opcode byte.
        bool canCompileLoopOSR(size_t loopStartOffset, size_t loopEndOffset,
                               const bytecode::BytecodeProgram& program,
                               uint8_t* outOpcode = nullptr) const;

        static const std::unordered_set<uint8_t>& getSupportedOpcodes();

        size_t compileCount = 0;
        size_t bailoutCount = 0;
        uint64_t inlineFieldICHits = 0;
        uint64_t inlineFieldICMisses = 0;
        uint64_t inlineFieldSetICHits = 0;
        uint64_t inlineFieldSetICMisses = 0;
        InlineDecisionCounters inlineDecisions;
    };
}
