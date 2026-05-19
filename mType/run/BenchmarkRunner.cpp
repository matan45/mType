#include "BenchmarkRunner.hpp"
#include "BenchmarkRunner_Internal.hpp"
#include <cstddef>

#include "../services/ScriptInterpreter.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
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
    using detail::JitFailedLoop;
    using detail::JitHotFunc;
    using detail::JitSample;
    using detail::IterationSample;
    using detail::ScriptResult;

    // Walks CWD upward up to 8 levels looking for `mType/tests/testFiles/benchmarks`.
    std::string locateBenchmarksDir()
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        fs::path cur = fs::current_path(ec);
        if (ec) return {};

        for (int depth = 0; depth < 8; ++depth)
        {
            fs::path candidate = cur / detail::BENCHMARKS_REL;
            if (fs::exists(candidate, ec) && fs::is_directory(candidate, ec))
            {
                return candidate.string();
            }
            if (!cur.has_parent_path() || cur == cur.parent_path()) break;
        cur = cur.parent_path();
        }
        return {};
    }

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

    std::string formatMs(double ms)
    {
        std::ostringstream os;
        os << std::fixed << std::setprecision(2) << ms;
        return os.str();
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
        out.reserve(detail::CANONICAL_SCRIPTS.size());
        for (const char* name : detail::CANONICAL_SCRIPTS)
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
        std::cerr << "Error: could not locate " << detail::BENCHMARKS_REL
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

        if (textMode) detail::printTextResult(r, options.jitEnabled);
        results.push_back(std::move(r));
    }

    if (textMode)
    {
        detail::printTextSummary(results, options.jitEnabled);
        std::cout << "Paste this block into docs/bench-baseline.md under today's date.\n";
    }
    else
    {
        detail::printJsonResults(results, options.jitEnabled);
    }

    return failed > 0 ? 1 : 0;
}

}
