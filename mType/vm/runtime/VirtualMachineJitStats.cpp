#include "VirtualMachine.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include "../jit/JitCodeCache.hpp"
#include "../jit/JitCompiler.hpp"
#include "../jit/JitProfiler.hpp"
#include "../jit/LoopProfiler.hpp"
#include "../jit/OSRManager.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../bytecode/OpCode.hpp"

namespace vm::runtime
{
    void VirtualMachine::printJitStats() const
    {
        std::cout << "\n=== JIT Statistics ===\n";

        if (!jitEnabled)
        {
            std::cout << "  JIT disabled\n";
            std::cout << "======================\n";
            return;
        }

        printJitFunctionProfilingStats();
        printJitCompilationStats();
        printJitFieldIcStats();
        printJitInlineDecisionStats();
        printJitLoopOsrStats();

        std::cout << "======================\n";
    }

    void VirtualMachine::printJitFunctionProfilingStats() const
    {
        std::cout << "Function Profiling:\n";
        if (!jitProfiler) return;

        std::cout << "  Hot threshold:          " << jitProfiler->getHotThreshold() << " calls\n";
        const auto& hotFuncs = jitProfiler->getHotFunctions();
        std::cout << "  Hot functions:          " << hotFuncs.size() << "\n";
        for (const auto& name : hotFuncs)
        {
            uint32_t calls = jitProfiler->getInvocationCount(name);
            bool compiled = jitCodeCache && jitCodeCache->contains(name);
            std::cout << "    - " << name << " (" << calls << " calls)"
                      << (compiled ? " [compiled]" : " [bailout]") << "\n";
        }
    }

    void VirtualMachine::printJitCompilationStats() const
    {
        std::cout << "Compilation:\n";
        if (jitCompiler)
        {
            std::cout << "  Successful compiles:    " << jitCompiler->getCompileCount() << "\n";
            std::cout << "  Bailouts:               " << jitCompiler->getBailoutCount() << "\n";
            auto printOpcodeBailouts = [](const char* label,
                                          const std::array<uint64_t, 256>& counts)
            {
                bool any = false;
                for (uint64_t count : counts)
                {
                    if (count != 0) { any = true; break; }
                }
                if (!any) return;

                std::cout << label << "\n";
                for (size_t i = 0; i < counts.size(); ++i)
                {
                    if (counts[i] == 0) continue;
                    std::cout << "    - "
                              << bytecode::getOpCodeName(static_cast<bytecode::OpCode>(i))
                              << ": " << counts[i] << "\n";
                }
            };
            printOpcodeBailouts("  Function bailout opcodes:",
                                jitCompiler->getFunctionBailoutOpcodes());
            printOpcodeBailouts("  OSR bailout opcodes:",
                                jitCompiler->getOSRBailoutOpcodes());
            // MYT-207: self-recursion call-dispatch telemetry.
            //   Tail calls optimized — `return self(args...)` sites lowered to
            //     arg-overwrite + jmp (loop). Counts CALL SITES, not dynamic
            //     iterations.
            //   Self direct calls — non-tail self-recursive sites lowered to
            //     a direct asmjit invoke against funcNode->label(), skipping
            //     the jit_call_function dispatch overhead.
            std::cout << "  Tail calls optimized:   " << jitCompiler->getTailCallsOptimized() << "\n";
            std::cout << "  Self direct calls:      " << jitCompiler->getSelfDirectCalls() << "\n";
        }
        if (jitCodeCache)
            std::cout << "  Cached functions:       " << jitCodeCache->size() << "\n";
    }

    void VirtualMachine::printJitFieldIcStats() const
    {
        // MYT-172 AC #3: inline field-IC stats. Hits = JIT fast-path took
        // the inlined shape-guard + indexed load/store. Misses = shape
        // guard failed and the slow-path helper handled the access.
        // MYT-191: SET counters split out separately — GET misses are
        // dominated by non-primitive tags, SET misses by ValueObject CoW,
        // merging them hides regressions.
        if (!jitCompiler) return;

        std::cout << "Field IC (GET):\n";
        std::cout << "  Inline hits:            "
                  << jitCompiler->getInlineFieldICHits() << "\n";
        std::cout << "  Inline misses:          "
                  << jitCompiler->getInlineFieldICMisses() << "\n";
        std::cout << "Field IC (SET):\n";
        std::cout << "  Inline hits:            "
                  << jitCompiler->getInlineFieldSetICHits() << "\n";
        std::cout << "  Inline misses:          "
                  << jitCompiler->getInlineFieldSetICMisses() << "\n";
    }

    void VirtualMachine::printJitInlineDecisionStats() const
    {
        // MYT-210 (fills MYT-179 stub): per-decision inline-eligibility
        // breakdown for both method-call and plain-function inlining.
        // Print only non-zero rows to keep output compact; if every row
        // is zero (no JIT-compiled function reached an inlineable site),
        // skip the section entirely.
        if (!jitCompiler) return;

        const auto& dec = jitCompiler->getInlineDecisions();
        auto anyNonZero = [](const std::array<uint64_t, jit::InlineDecisionCounters::SIZE>& a) {
            for (auto v : a) if (v) return true;
            return false;
        };
        auto printSection = [](const char* label,
                                const std::array<uint64_t, jit::InlineDecisionCounters::SIZE>& a) {
            std::cout << label << "\n";
            for (size_t i = 0; i < jit::InlineDecisionCounters::SIZE; ++i)
            {
                if (!a[i]) continue;
                const auto reason = static_cast<optimization::InlineDecision>(i);
                std::cout << "  " << optimization::inlineDecisionName(reason)
                          << ": " << a[i] << "\n";
            }
        };
        if (anyNonZero(dec.perReasonMethod))
            printSection("Inline decisions (method):", dec.perReasonMethod);
        if (anyNonZero(dec.perReasonFunction))
            printSection("Inline decisions (function):", dec.perReasonFunction);
    }

    void VirtualMachine::printJitLoopOsrStats() const
    {
        std::cout << "Loop OSR:\n";
        if (!osrManager) return;

        const auto& loopProfiler = osrManager->getLoopProfiler();
        const auto& profiles = loopProfiler.getProfiles();
        size_t compiled = 0, failed = 0;
        for (const auto& [id, profile] : profiles)
        {
            if (profile.osrCompiled) compiled++;
            if (profile.osrFailed) failed++;
        }
        std::cout << "  OSR threshold:          " << loopProfiler.getOsrThreshold() << " iterations\n";
        std::cout << "  Loops profiled:         " << profiles.size() << "\n";
        std::cout << "  OSR compiled:           " << compiled << "\n";
        std::cout << "  OSR failed:             " << failed << "\n";

        // MYT-148: per-loop bailout reason breakdown. Answers "why didn't
        // this loop tier up?" without a debugger. Shows the offending
        // opcode mnemonic for UNSUPPORTED_OPCODE / CODEGEN_FAILURE so the
        // next step (remediation) is obvious.
        if (failed == 0) return;

        std::cout << "  Failed loops:\n";
        for (const auto& [id, profile] : profiles)
        {
            if (!profile.osrFailed) continue;
            std::cout << "    - offset 0x" << std::hex << id.jumpBackOffset
                      << std::dec << ": "
                      << jit::osrBailoutReasonName(profile.bailoutReason);
            if (profile.bailoutReason == jit::OSRBailoutReason::UNSUPPORTED_OPCODE ||
                profile.bailoutReason == jit::OSRBailoutReason::CODEGEN_FAILURE)
            {
                std::cout << " (0x" << std::hex
                          << static_cast<unsigned>(profile.offendingOpcode)
                          << std::dec << " = "
                          << bytecode::getOpCodeName(
                                 static_cast<bytecode::OpCode>(profile.offendingOpcode))
                          << ")";
            }
            std::cout << "\n";
        }
    }
}
