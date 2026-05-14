#include "BenchmarkRunner.hpp"
#include <cstddef>

#include "../services/ScriptInterpreter.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../vm/runtime/context/ExecutionContext.hpp"
#include "../vm/bytecode/OpCode.hpp"
#include "../vm/jit/JitCompiler.hpp"
#include "../vm/jit/JitCodeCache.hpp"
#include "../vm/jit/JitProfiler.hpp"
#include "../vm/jit/OSRManager.hpp"
#include "../vm/jit/LoopProfiler.hpp"
#include "../value/StringPool.hpp"
#include "../value/ArrayPool.hpp"
#include "../gc/GC.hpp"
#include "../lexer/Lexer.hpp"
#include "../token/TokenType.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace runMain
{
namespace
{
    constexpr std::array<const char*, 46> CANONICAL_SCRIPTS = {
        "arithmetic_tight_loop.mt",
        "method_dispatch.mt",
        "object_alloc.mt",
        // MYT-208: short-scope allocations inside a helper called in a hot
        // loop — the workload class where escape-analysis Phase 2 wins.
        "object_alloc_nested.mt",
        // MYT-191: direct-field-write hot loop. SET_FIELD lives inside the
        // OSR-compiled outer loop (unlike object_alloc.mt, whose SETs are
        // in Point.constructor and run interpreted), so this is the only
        // canonical script that exercises tryEmitInlinedFieldSet.
        "field_write_hot.mt",
        // MYT-194: mirror of field_write_hot for the GET path. Primary
        // acceptance benchmark for the GET_FIELD_CACHED interpreter fast
        // path (and tryEmitInlinedFieldGet on the JIT side).
        "field_read_hot.mt",
        "polymorphic_field_hot.mt",
        "string_ops.mt",
        "recursive.mt",
        "bitwise_tight_loop.mt",
        "short_circuit_chain.mt",
        "primitive_method_dispatch.mt",
        "array_multi_alloc.mt",
        "array_multi_get.mt",
        "for_each_loop.mt",
        // MYT-163 F-a: primary acceptance target — single-class MONO call
        // site eligible for speculative inlining.
        "inline_monomorphic.mt",
        // MYT-164 F-b: MONO callee with an internal if-guard (mirrors the
        // iterator hasNext / null-check pattern). Un-inlineable under F-a
        // due to HAS_INTERNAL_JUMPS; inlined under F-b via per-frame label
        // remapping.
        "inline_branching.mt",
        // MYT-165 F-c: 4-subclass polymorphic hot loop (fills
        // IC_MAX_POLYMORPHIC_ENTRIES = 4). Exercises the chained shape-guard
        // emission against the maximum IC-width case.
        "inline_polymorphic.mt",
        // MYT-173 follow-up: mixed-inlineability POLY-4 site. Three small
        // shapes inline, one oversized shape routes through the per-shape
        // helper branch in emitInlinedMethodCallPoly. Pre-change every
        // dispatch ran jit_call_method_ic because the all-or-nothing
        // eligibility gate rejected the whole site; primary regression
        // benchmark for the per-entry POLY eligibility relaxation.
        "inline_polymorphic_mixed.mt",
        // MYT-173 follow-up: 6 subclasses overflow IC_MAX_POLYMORPHIC_ENTRIES
        // (= 4) and drive the IC to MEGAMORPHIC. Baseline for the MEGA cliff —
        // every dispatch hits jit_call_method_ic with no inlined fast path.
        "megamorphic_dispatch.mt",
        // MYT-167 F-e: value-class MONO hot loop. Pre-F-e the slow path was
        // jit_call_method's temp-ObjectInstance materialisation per call;
        // post-F-e the IC populates with receiverIsValueObject=true and the
        // body inlines identically to inline_monomorphic.mt.
        "inline_value_object_hot.mt",
        // MYT-210: plain-CALL inliner acceptance benchmark. 4-arg leaf
        // distanceSq in a tight 2 M-iter loop — measures the per-call
        // overhead removed by inlining versus the jit_call_function helper.
        "function_call_hot.mt",
        // Async/await suspend-resume cost in a tight await loop.
        "async_await_tight_loop.mt",
        // Sequential await chain — 4 awaits/iter, distinct continuation shapes.
        "async_await_chain.mt",
        // Lambda invocation through a single-method interface in a hot loop.
        "lambda_call_hot.mt",
        // Closure-capture read overhead on the lambda hot path.
        "lambda_closure_hot.mt",
        // MYT-228: hot loop dispatching `isClassOf T` through a method-level
        // generic helper. Exercises BIND_TYPE_ARGS staging,
        // CallFrame::typeArgBindings consume, and the
        // jit_instanceof_typeparam JIT helper. Acceptance: per-call overhead
        // stays close to method_dispatch.mt — the per-frame map lookup is
        // not on the non-generic hot path, so non-generic benchmarks must
        // not regress.
        "generic_dispatch_hot.mt",
        // Runtime feature-surface coverage beyond the core VM hot paths above.
        "try_catch_finally_hot.mt",
        "switch_dispatch_hot.mt",
        "overload_dispatch_hot.mt",
        "abstract_dispatch_hot.mt",
        "cast_hot.mt",
        "collections_hash_hot.mt",
        "collections_hash_user_class_hot.mt",
        "collections_hashset_hot.mt",
        "stream_pipeline_hot.mt",
        "reflection_lookup_hot.mt",
        "pattern_match_hot.mt",
        "string_interpolation_hot.mt",
        "boxed_primitive_dispatch_hot.mt",
        // Isolation benchmarks for the boxed-primitive INVOKE_BOOL_* /
        // INVOKE_STRING_* fast paths and the StringPool intern hit rate.
        // Companions to boxed_primitive_dispatch_hot.mt — staying pure-Bool
        // / pure-String makes regressions in either path attributable.
        "boxed_bool_dispatch_hot.mt",
        "boxed_string_dispatch_hot.mt",
        // Isolation benchmark for non-generic CALL_STATIC dispatch.
        // Companion to generic_dispatch_hot.mt; strips BIND_TYPE_ARGS and
        // INSTANCEOF_TYPEPARAM so the measured cost is purely the
        // jit_call_function_ic + interpreter-side cached-static-call path.
        // Should land near method_dispatch.mt (~125ms) once warmed.
        "static_call_hot.mt",
        // MYT-254: linked-list traversal under nested loops. Inner walks
        // all M elements via get(k), which chases Node.next pointers each
        // call — O(M^2) per outer iter, exercising field-chain inlining
        // at depth 2+ on a linked container (vs the ArrayList-backed
        // stream_pipeline_hot).
        "linked_list_nested_hot.mt",
        "method_chain_hot.mt",
        "string_build_call_hot.mt",
    };

    constexpr const char* BENCHMARKS_REL = "mType/tests/testFiles/benchmarks";

    struct JitFailedLoop
    {
        std::size_t jumpBackOffset = 0;
        vm::jit::OSRBailoutReason reason = vm::jit::OSRBailoutReason::NONE;
        std::uint8_t offendingOpcode = 0;
    };

    struct JitHotFunc
    {
        std::string name;
        std::uint32_t calls = 0;
        bool compiled = false;
    };

    struct JitSample
    {
        bool captured = false;
        std::size_t compileCount = 0;
        std::size_t bailoutCount = 0;
        std::size_t cachedFunctions = 0;
        std::size_t loopsProfiled = 0;
        std::size_t osrCompiled = 0;
        std::size_t osrFailed = 0;
        std::uint64_t inlineFieldICHits = 0;
        std::uint64_t inlineFieldICMisses = 0;
        std::array<std::uint64_t, 256> functionBailoutOpcodes = {};
        std::array<std::uint64_t, 256> osrBailoutOpcodes = {};
        std::vector<JitFailedLoop> failedLoops;
        std::vector<JitHotFunc> hotFunctions;
    };

    struct IterationSample
    {
        double wallMs = 0.0;
        vm::runtime::ExecutionStats stats{};
        std::size_t stringPoolRequests = 0;
        std::size_t stringPoolHits = 0;
        std::size_t arrayPoolAllocs = 0;
        std::size_t arrayPoolHits = 0;
        JitSample jit{};
    };

    JitSample captureJitSample(const vm::runtime::VirtualMachine& vm)
    {
        JitSample out{};
        out.captured = true;

        if (auto* compiler = vm.getJitCompiler())
        {
            out.compileCount = compiler->getCompileCount();
            out.bailoutCount = compiler->getBailoutCount();
            out.inlineFieldICHits = compiler->getInlineFieldICHits();
            out.inlineFieldICMisses = compiler->getInlineFieldICMisses();
            out.functionBailoutOpcodes = compiler->getFunctionBailoutOpcodes();
            out.osrBailoutOpcodes = compiler->getOSRBailoutOpcodes();
        }
        if (auto* cache = vm.getJitCodeCache())
        {
            out.cachedFunctions = cache->size();
        }
        if (auto* profiler = vm.getJitProfiler())
        {
            const auto& hot = profiler->getHotFunctions();
            out.hotFunctions.reserve(hot.size());
            auto* cache = vm.getJitCodeCache();
            for (const auto& name : hot)
            {
                JitHotFunc hf;
                hf.name = name;
                hf.calls = profiler->getInvocationCount(name);
                hf.compiled = cache && cache->contains(name);
                out.hotFunctions.push_back(std::move(hf));
            }
        }
        if (auto* osr = vm.getOSRManager())
        {
            const auto& profiles = osr->getLoopProfiler().getProfiles();
            out.loopsProfiled = profiles.size();
            for (const auto& [id, profile] : profiles)
            {
                if (profile.osrCompiled) ++out.osrCompiled;
                if (profile.osrFailed)
                {
                    ++out.osrFailed;
                    JitFailedLoop f;
                    f.jumpBackOffset = id.jumpBackOffset;
                    f.reason = profile.bailoutReason;
                    f.offendingOpcode = profile.offendingOpcode;
                    out.failedLoops.push_back(f);
                }
            }
        }
        return out;
    }

    struct ScriptResult
    {
        std::string path;
        std::string name;
        bool ok = true;
        std::string errorMessage;
        std::vector<IterationSample> measured;
    };

    struct Aggregate
    {
        double minMs = 0.0;
        double medianMs = 0.0;
        double meanMs = 0.0;
        double stddevMs = 0.0;
    };

    // Walks CWD upward up to 8 levels looking for `mType/tests/testFiles/benchmarks`.
    std::string locateBenchmarksDir()
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        fs::path cur = fs::current_path(ec);
        if (ec) return {};

        for (int depth = 0; depth < 8; ++depth)
        {
            fs::path candidate = cur / BENCHMARKS_REL;
            if (fs::exists(candidate, ec) && fs::is_directory(candidate, ec))
            {
                return candidate.string();
            }
            if (!cur.has_parent_path() || cur == cur.parent_path()) break;
            cur = cur.parent_path();
        }
        return {};
    }

    IterationSample runOne(const std::string& path, bool jitEnabled, bool captureJit,
                           bool suppressScriptOutput)
    {
        IterationSample sample{};

        const auto stringPoolBefore = value::StringPool::getInstance().getStats();
        const auto arrayPoolBefore = value::ArrayPool::getInstance().getGlobalStats();

        services::ScriptInterpreter interpreter(constants::ExecutionMode::BYTECODE_VM);
        interpreter.getVM()->setJitEnabled(jitEnabled);

        const auto t0 = std::chrono::high_resolution_clock::now();
        std::ostringstream suppressedOutput;
        std::streambuf* oldCout = nullptr;
        if (suppressScriptOutput)
            oldCout = std::cout.rdbuf(suppressedOutput.rdbuf());
        try
        {
            interpreter.runScript(path);
        }
        catch (...)
        {
            if (oldCout) std::cout.rdbuf(oldCout);
            throw;
        }
        if (oldCout) std::cout.rdbuf(oldCout);
        const auto t1 = std::chrono::high_resolution_clock::now();

        sample.wallMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        sample.stats = interpreter.getVM()->getStats();

        if (captureJit && jitEnabled)
        {
            sample.jit = captureJitSample(*interpreter.getVM());
        }

        gc::GC::forceCollect();

        const auto stringPoolAfter = value::StringPool::getInstance().getStats();
        const auto arrayPoolAfter = value::ArrayPool::getInstance().getGlobalStats();

        sample.stringPoolRequests = stringPoolAfter.totalRequests - stringPoolBefore.totalRequests;
        sample.stringPoolHits = stringPoolAfter.poolHits - stringPoolBefore.poolHits;
        sample.arrayPoolAllocs = arrayPoolAfter.totalAllocations - arrayPoolBefore.totalAllocations;
        sample.arrayPoolHits = arrayPoolAfter.poolHits - arrayPoolBefore.poolHits;

        return sample;
    }

    Aggregate summarize(const std::vector<IterationSample>& samples)
    {
        Aggregate agg{};
        if (samples.empty()) return agg;

        std::vector<double> wall;
        wall.reserve(samples.size());
        for (const auto& s : samples) wall.push_back(s.wallMs);

        std::sort(wall.begin(), wall.end());
        agg.minMs = wall.front();

        const std::size_t n = wall.size();
        agg.medianMs = (n % 2 == 0) ? (wall[n / 2 - 1] + wall[n / 2]) / 2.0 : wall[n / 2];

        double sum = 0.0;
        for (double v : wall) sum += v;
        agg.meanMs = sum / static_cast<double>(n);

        if (n >= 2)
        {
            double sqSum = 0.0;
            for (double v : wall)
            {
                const double d = v - agg.meanMs;
                sqSum += d * d;
            }
            agg.stddevMs = std::sqrt(sqSum / static_cast<double>(n - 1));
        }

        return agg;
    }

    std::string formatMs(double ms)
    {
        std::ostringstream os;
        os << std::fixed << std::setprecision(2) << ms;
        return os.str();
    }

    std::string jsonEscape(const std::string& s)
    {
        std::ostringstream os;
        for (char ch : s)
        {
            switch (ch)
            {
                case '\\': os << "\\\\"; break;
                case '"': os << "\\\""; break;
                case '\n': os << "\\n"; break;
                case '\r': os << "\\r"; break;
                case '\t': os << "\\t"; break;
                default: os << ch; break;
            }
        }
        return os.str();
    }

    void printTextJit(const JitSample& j)
    {
        if (!j.captured) return;

        std::cout << "    jit            compiled=" << j.compileCount
                  << " bailouts=" << j.bailoutCount
                  << " cached=" << j.cachedFunctions
                  << " osr-profiled=" << j.loopsProfiled
                  << " osr-compiled=" << j.osrCompiled
                  << " osr-failed=" << j.osrFailed
                  << " field-ic-hits=" << j.inlineFieldICHits
                  << " field-ic-misses=" << j.inlineFieldICMisses << "\n";

        auto printOpcodeCounts = [](const char* label,
                                    const std::array<std::uint64_t, 256>& counts)
        {
            bool any = false;
            for (std::uint64_t count : counts)
            {
                if (count != 0) { any = true; break; }
            }
            if (!any) return;

            std::cout << "      " << label << "\n";
            for (std::size_t i = 0; i < counts.size(); ++i)
            {
                if (counts[i] == 0) continue;
                std::cout << "        "
                          << vm::bytecode::getOpCodeName(
                                 static_cast<vm::bytecode::OpCode>(i))
                          << ": " << counts[i] << "\n";
            }
        };
        printOpcodeCounts("function-bailout-opcodes", j.functionBailoutOpcodes);
        printOpcodeCounts("osr-bailout-opcodes", j.osrBailoutOpcodes);

        if (!j.hotFunctions.empty())
        {
            for (const auto& hf : j.hotFunctions)
            {
                std::cout << "      hot " << hf.name
                          << " (" << hf.calls << " calls) "
                          << (hf.compiled ? "[compiled]" : "[bailout]") << "\n";
            }
        }

        for (const auto& f : j.failedLoops)
        {
            std::cout << "      osr-fail offset 0x" << std::hex << f.jumpBackOffset
                      << std::dec << ": " << vm::jit::osrBailoutReasonName(f.reason);
            if (f.reason == vm::jit::OSRBailoutReason::UNSUPPORTED_OPCODE ||
                f.reason == vm::jit::OSRBailoutReason::CODEGEN_FAILURE)
            {
                std::cout << " (" << vm::bytecode::getOpCodeName(
                                  static_cast<vm::bytecode::OpCode>(f.offendingOpcode))
                          << ")";
            }
            std::cout << "\n";
        }
    }

    void printJsonOpcodeCounts(const char* fieldName,
                               const std::array<std::uint64_t, 256>& counts,
                               const char* suffix)
    {
        std::cout << "          \"" << fieldName << "\": [";
        bool first = true;
        for (std::size_t i = 0; i < counts.size(); ++i)
        {
            if (counts[i] == 0) continue;
            if (!first) std::cout << ", ";
            first = false;
            std::cout << "{ \"opcode\": \""
                      << vm::bytecode::getOpCodeName(static_cast<vm::bytecode::OpCode>(i))
                      << "\", \"count\": " << counts[i] << " }";
        }
        std::cout << "]" << suffix << "\n";
    }

    void printTextResult(const ScriptResult& r, bool jitEnabled)
    {
        if (!r.ok)
        {
            std::cout << "  [FAILED] " << r.name << ": " << r.errorMessage << "\n\n";
            return;
        }
        const Aggregate agg = summarize(r.measured);
        const auto& first = r.measured.front();

        std::cout << "  " << r.name << "  (jit=" << (jitEnabled ? "on" : "off") << ")\n";
        std::cout << "    wall-clock ms  min=" << formatMs(agg.minMs)
                  << "  median=" << formatMs(agg.medianMs)
                  << "  mean=" << formatMs(agg.meanMs)
                  << "  stddev=" << formatMs(agg.stddevMs) << "\n";
        std::cout << "    exec (VM)      "
                  << formatMs(static_cast<double>(first.stats.executionTime.count()) / 1000.0)
                  << " ms\n";
        std::cout << "    instructions   " << first.stats.instructionsExecuted << "\n";
        std::cout << "    calls          " << first.stats.functionCalls << "\n";
        std::cout << "    mem-allocated  " << first.stats.memoryAllocated << " (unused stub)\n";
        std::cout << "    string-pool    req=" << first.stringPoolRequests
                  << " hits=" << first.stringPoolHits << "\n";
        std::cout << "    array-pool     alloc=" << first.arrayPoolAllocs
                  << " hits=" << first.arrayPoolHits << "\n";
        printTextJit(first.jit);
        std::cout << "\n";
    }

    void printTextSummary(const std::vector<ScriptResult>& results, bool jitEnabled)
    {
        std::cout << "=== Summary (jit=" << (jitEnabled ? "on" : "off") << ") ===\n";
        std::cout << "  "
                  << std::left << std::setw(28) << "Script"
                  << std::right << std::setw(14) << "min(ms)"
                  << std::right << std::setw(14) << "median(ms)"
                  << std::right << std::setw(16) << "instructions"
                  << std::right << std::setw(10) << "calls"
                  << "\n";
        for (const auto& r : results)
        {
            if (!r.ok)
            {
                std::cout << "  "
                          << std::left << std::setw(28) << r.name
                          << std::right << std::setw(14) << "FAILED"
                          << "\n";
                continue;
            }
            const Aggregate agg = summarize(r.measured);
            const auto& first = r.measured.front();
            std::cout << "  "
                      << std::left << std::setw(28) << r.name
                      << std::right << std::setw(14) << formatMs(agg.minMs)
                      << std::right << std::setw(14) << formatMs(agg.medianMs)
                      << std::right << std::setw(16) << first.stats.instructionsExecuted
                      << std::right << std::setw(10) << first.stats.functionCalls
                      << "\n";
        }
        std::cout << "\n";
    }

    void printJsonResults(const std::vector<ScriptResult>& results, bool jitEnabled)
    {
        std::cout << "{\n  \"jit\": " << (jitEnabled ? "true" : "false") << ",\n";
        std::cout << "  \"scripts\": [\n";
        for (std::size_t i = 0; i < results.size(); ++i)
        {
            const auto& r = results[i];
            std::cout << "    {\n";
            std::cout << "      \"name\": \"" << jsonEscape(r.name) << "\",\n";
            std::cout << "      \"path\": \"" << jsonEscape(r.path) << "\",\n";
            std::cout << "      \"ok\": " << (r.ok ? "true" : "false");
            if (!r.ok)
            {
                std::cout << ",\n      \"error\": \"" << jsonEscape(r.errorMessage) << "\"";
                std::cout << "\n    }" << (i + 1 < results.size() ? "," : "") << "\n";
                continue;
            }
            const Aggregate agg = summarize(r.measured);
            const auto& first = r.measured.front();
            std::cout << ",\n      \"iterations\": " << r.measured.size() << ",\n";
            std::cout << "      \"wall_ms\": { \"min\": " << formatMs(agg.minMs)
                      << ", \"median\": " << formatMs(agg.medianMs)
                      << ", \"mean\": " << formatMs(agg.meanMs)
                      << ", \"stddev\": " << formatMs(agg.stddevMs) << " },\n";
            std::cout << "      \"exec_ms\": "
                      << formatMs(static_cast<double>(first.stats.executionTime.count()) / 1000.0) << ",\n";
            std::cout << "      \"instructions\": " << first.stats.instructionsExecuted << ",\n";
            std::cout << "      \"calls\": " << first.stats.functionCalls << ",\n";
            std::cout << "      \"string_pool\": { \"requests\": " << first.stringPoolRequests
                      << ", \"hits\": " << first.stringPoolHits << " },\n";
            std::cout << "      \"array_pool\": { \"allocations\": " << first.arrayPoolAllocs
                      << ", \"hits\": " << first.arrayPoolHits << " }";
            if (first.jit.captured)
            {
                std::cout << ",\n      \"jit\": {\n";
                std::cout << "        \"compiled\": " << first.jit.compileCount << ",\n";
                std::cout << "        \"bailouts\": " << first.jit.bailoutCount << ",\n";
                std::cout << "        \"cached\": " << first.jit.cachedFunctions << ",\n";
                std::cout << "        \"osr_profiled\": " << first.jit.loopsProfiled << ",\n";
                std::cout << "        \"osr_compiled\": " << first.jit.osrCompiled << ",\n";
                std::cout << "        \"osr_failed\": " << first.jit.osrFailed << ",\n";
                std::cout << "        \"field_ic\": { \"hits\": "
                          << first.jit.inlineFieldICHits << ", \"misses\": "
                          << first.jit.inlineFieldICMisses << " },\n";
                printJsonOpcodeCounts("function_bailout_opcodes",
                                      first.jit.functionBailoutOpcodes, ",");
                printJsonOpcodeCounts("osr_bailout_opcodes",
                                      first.jit.osrBailoutOpcodes, ",");
                std::cout << "        \"hot_functions\": [";
                for (std::size_t h = 0; h < first.jit.hotFunctions.size(); ++h)
                {
                    const auto& hf = first.jit.hotFunctions[h];
                    if (h != 0) std::cout << ", ";
                    std::cout << "{ \"name\": \"" << jsonEscape(hf.name)
                              << "\", \"calls\": " << hf.calls
                              << ", \"compiled\": " << (hf.compiled ? "true" : "false")
                              << " }";
                }
                std::cout << "],\n";
                std::cout << "        \"failed_loops\": [";
                for (std::size_t f = 0; f < first.jit.failedLoops.size(); ++f)
                {
                    const auto& fl = first.jit.failedLoops[f];
                    if (f != 0) std::cout << ", ";
                    std::cout << "{ \"jump_back_offset\": " << fl.jumpBackOffset
                              << ", \"reason\": \""
                              << vm::jit::osrBailoutReasonName(fl.reason)
                              << "\", \"opcode\": \""
                              << vm::bytecode::getOpCodeName(
                                     static_cast<vm::bytecode::OpCode>(fl.offendingOpcode))
                              << "\" }";
                }
                std::cout << "]\n";
                std::cout << "      }";
            }
            std::cout << "\n";
            std::cout << "    }" << (i + 1 < results.size() ? "," : "") << "\n";
        }
        std::cout << "  ]\n}\n";
    }

    ScriptResult runScript(const std::string& path, const BenchmarkOptions& opts)
    {
        ScriptResult result;
        result.path = path;
        result.name = std::filesystem::path(path).filename().string();

        try
        {
            for (int i = 0; i < opts.warmupIterations; ++i)
            {
                const bool suppressScriptOutput = opts.outputFormat == BenchmarkOutput::Json;
                (void)runOne(path, opts.jitEnabled, false, suppressScriptOutput);
            }
            for (int i = 0; i < opts.measuredIterations; ++i)
            {
                // Only capture JIT stats on the first measured iteration — the
                // runner recreates the VM per iteration so stats don't accumulate,
                // and the printed report reads from samples[0] anyway.
                const bool captureJit = opts.printJitStats && i == 0;
                const bool suppressScriptOutput = opts.outputFormat == BenchmarkOutput::Json;
                result.measured.push_back(runOne(path, opts.jitEnabled, captureJit,
                                                 suppressScriptOutput));
            }
        }
        catch (const std::exception& e)
        {
            result.ok = false;
            result.errorMessage = e.what();
        }
        catch (...)
        {
            result.ok = false;
            result.errorMessage = "(unknown exception)";
        }
        return result;
    }

    struct LexerIterationSample
    {
        double wallMs = 0.0;
        std::size_t tokenCount = 0;
    };

    LexerIterationSample runLexerOne(const std::string& path)
    {
        LexerIterationSample s{};
        lexer::Lexer lexer(path);

        const auto t0 = std::chrono::high_resolution_clock::now();
        for (;;)
        {
            token::Token tok = lexer.getNextToken();
            ++s.tokenCount;
            if (tok.type == token::TokenType::END) break;
        }
        const auto t1 = std::chrono::high_resolution_clock::now();

        s.wallMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        return s;
    }

    int runLexerBenchmark(const BenchmarkOptions& opts)
    {
        const std::string& path = opts.singleLexerScript;

        std::error_code ec;
        if (!std::filesystem::exists(path, ec))
        {
            std::cerr << "Error: lexer benchmark input not found: " << path << "\n";
            return 2;
        }

        std::cout << "mType lexer microbenchmark  (warmup=" << opts.warmupIterations
                  << ", measured=" << opts.measuredIterations << ")\n";
        std::cout << "  input: " << path << "\n\n";

        try
        {
            for (int i = 0; i < opts.warmupIterations; ++i)
            {
                (void)runLexerOne(path);
            }

            std::vector<LexerIterationSample> samples;
            samples.reserve(opts.measuredIterations);
            for (int i = 0; i < opts.measuredIterations; ++i)
            {
                samples.push_back(runLexerOne(path));
            }

            std::vector<double> wall;
            wall.reserve(samples.size());
            for (const auto& s : samples) wall.push_back(s.wallMs);
            std::sort(wall.begin(), wall.end());

            const double minMs = wall.front();
            const std::size_t n = wall.size();
            const double medianMs = (n % 2 == 0) ? (wall[n / 2 - 1] + wall[n / 2]) / 2.0 : wall[n / 2];
            double sum = 0.0;
            for (double v : wall) sum += v;
            const double meanMs = sum / static_cast<double>(n);
            double sqSum = 0.0;
            for (double v : wall)
            {
                const double d = v - meanMs;
                sqSum += d * d;
            }
            const double stddevMs = (n >= 2) ? std::sqrt(sqSum / static_cast<double>(n - 1)) : 0.0;

            const std::size_t tokens = samples.front().tokenCount;
            const double tokensPerSecMin = (minMs > 0.0)
                ? static_cast<double>(tokens) / (minMs / 1000.0)
                : 0.0;

            std::cout << "  tokens         " << tokens << "\n";
            std::cout << "  wall-clock ms  min=" << formatMs(minMs)
                      << "  median=" << formatMs(medianMs)
                      << "  mean=" << formatMs(meanMs)
                      << "  stddev=" << formatMs(stddevMs) << "\n";
            std::cout << "  tokens/sec     " << std::fixed << std::setprecision(0)
                      << tokensPerSecMin << " (at min wall)\n\n";
            return 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: lexer benchmark failed: " << e.what() << "\n";
            return 1;
        }
    }

    std::vector<std::string> resolveScriptPaths(const BenchmarkOptions& opts)
    {
        if (!opts.singleScript.empty())
        {
            return { opts.singleScript };
        }

        const std::string dir = locateBenchmarksDir();
        if (dir.empty()) return {};

        std::vector<std::string> out;
        out.reserve(CANONICAL_SCRIPTS.size());
        for (const char* name : CANONICAL_SCRIPTS)
        {
            out.push_back((std::filesystem::path(dir) / name).string());
        }
        return out;
    }
}

int runBenchmarks(const BenchmarkOptions& options)
{
    if (!options.singleLexerScript.empty())
    {
        return runLexerBenchmark(options);
    }

    const auto paths = resolveScriptPaths(options);
    if (paths.empty())
    {
        std::cerr << "Error: could not locate " << BENCHMARKS_REL
                  << " (searched parents of current directory).\n";
        std::cerr << "Run from the repo root, or pass --benchmark=<path>.\n";
        return 2;
    }

    const bool textMode = (options.outputFormat == BenchmarkOutput::Text);

    if (textMode)
    {
        std::cout << "mType benchmark suite  (jit=" << (options.jitEnabled ? "on" : "off")
                  << ", warmup=" << options.warmupIterations
                  << ", measured=" << options.measuredIterations << ")\n\n";
    }

    std::vector<ScriptResult> results;
    results.reserve(paths.size());
    int failed = 0;

    for (const auto& p : paths)
    {
        if (textMode)
        {
            std::cout << "--- running " << std::filesystem::path(p).filename().string()
                      << " ---\n";
        }
        auto r = runScript(p, options);
        if (!r.ok) ++failed;

        if (textMode) printTextResult(r, options.jitEnabled);
        results.push_back(std::move(r));
    }

    if (textMode)
    {
        printTextSummary(results, options.jitEnabled);
        std::cout << "Paste this block into docs/bench-baseline.md under today's date.\n";
    }
    else
    {
        printJsonResults(results, options.jitEnabled);
    }

    return failed > 0 ? 1 : 0;
}

} // namespace runMain
