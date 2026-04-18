#pragma once
#include <string>

namespace runMain
{
    enum class BenchmarkOutput
    {
        Text,
        Json
    };

    struct BenchmarkOptions
    {
        // Empty = run the canonical suite from mType/tests/testFiles/benchmarks/.
        // Non-empty = run a single .mt file at this path.
        std::string singleScript;

        int measuredIterations = 3;
        int warmupIterations = 1;
        BenchmarkOutput outputFormat = BenchmarkOutput::Text;
        bool jitEnabled = true;
        bool printJitStats = false;
    };

    // Runs the benchmark suite (or a single script) according to options and
    // prints results to stdout. Returns 0 on success, 1 if any script failed
    // to run, 2 if the benchmarks directory could not be located.
    int runBenchmarks(const BenchmarkOptions& options);
}
