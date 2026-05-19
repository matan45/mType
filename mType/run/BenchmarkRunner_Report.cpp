#include "BenchmarkRunner_Internal.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "../vm/bytecode/OpCode.hpp"
#include "../vm/jit/OSRManager.hpp"

namespace runMain
{
namespace detail
{
namespace
{
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
}
}
