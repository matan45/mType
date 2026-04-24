#pragma once

#include <string>
#include <chrono>
#include <array>
#include <cstdint>

namespace vm::profiler
{
    struct FunctionProfile
    {
        std::string functionName;
        uint64_t callCount = 0;
        uint64_t totalTimeNs = 0;
        uint64_t selfTimeNs = 0;
    };

    struct CallGraphEdge
    {
        std::string caller;
        std::string callee;
        uint64_t callCount = 0;
        uint64_t totalTimeNs = 0;
    };

    struct TimingStackEntry
    {
        std::string functionName;
        std::chrono::high_resolution_clock::time_point entryTime;
        uint64_t childTimeNs = 0;
    };

    struct OpcodeProfile
    {
        std::array<uint64_t, 256> counts{};
        // MYT-202: adjacent-pair counts indexed by (prev<<8 | cur). 64 KiB;
        // only populated in FULL mode. Feeds the peephole-target selection.
        std::array<uint64_t, 65536> pairCounts{};
    };
}
