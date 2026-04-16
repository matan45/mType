#include "BenchmarkRunner.hpp"

#include "../services/ScriptInterpreter.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../vm/runtime/context/ExecutionContext.hpp"
#include "../value/StringPool.hpp"
#include "../value/ArrayPool.hpp"
#include "../gc/GC.hpp"

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
    constexpr std::array<const char*, 5> CANONICAL_SCRIPTS = {
        "arithmetic_tight_loop.mt",
        "method_dispatch.mt",
        "object_alloc.mt",
        "string_ops.mt",
        "recursive.mt",
    };

    constexpr const char* BENCHMARKS_REL = "mType/tests/testFiles/benchmarks";

    struct IterationSample
    {
        double wallMs = 0.0;
        vm::runtime::ExecutionStats stats{};
        std::size_t stringPoolRequests = 0;
        std::size_t stringPoolHits = 0;
        std::size_t arrayPoolAllocs = 0;
        std::size_t arrayPoolHits = 0;
    };

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

    IterationSample runOne(const std::string& path, bool jitEnabled)
    {
        IterationSample sample{};

        const auto stringPoolBefore = value::StringPool::getInstance().getStats();
        const auto arrayPoolBefore = value::ArrayPool::getInstance().getGlobalStats();

        services::ScriptInterpreter interpreter(constants::ExecutionMode::BYTECODE_VM);
        interpreter.getVM()->setJitEnabled(jitEnabled);

        const auto t0 = std::chrono::high_resolution_clock::now();
        interpreter.runScript(path);
        const auto t1 = std::chrono::high_resolution_clock::now();

        sample.wallMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        sample.stats = interpreter.getVM()->getStats();

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
                  << " hits=" << first.arrayPoolHits << "\n\n";
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
            std::cout << "      \"name\": \"" << r.name << "\",\n";
            std::cout << "      \"path\": \"" << r.path << "\",\n";
            std::cout << "      \"ok\": " << (r.ok ? "true" : "false");
            if (!r.ok)
            {
                std::cout << ",\n      \"error\": \"" << r.errorMessage << "\"";
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
                      << ", \"hits\": " << first.arrayPoolHits << " }\n";
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
                (void)runOne(path, opts.jitEnabled);
            }
            for (int i = 0; i < opts.measuredIterations; ++i)
            {
                result.measured.push_back(runOne(path, opts.jitEnabled));
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
