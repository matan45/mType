#pragma once

#include <string>
#include <cstddef>
#include <chrono>
#include <array>
#include <cstdint>
#include <memory>

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
        // MYT-202: adjacent-pair counts indexed by (prev<<8 | cur). 65536
        // slots × 8 bytes = 512 KiB — only useful in FULL mode, so allocate
        // lazily on first recordOpcodeExecuted call. Singleton storage
        // bounds this to one instance per process; still, reserving 512 KiB
        // unconditionally for runs that never enable FULL profiling is
        // wasteful. Callers must null-check (pairCounts ? ... : 0).
        std::unique_ptr<std::array<uint64_t, 65536>> pairCounts;

        static constexpr size_t pairCountsSize() { return 65536; }
        uint64_t pairCountAt(size_t i) const {
            return pairCounts ? (*pairCounts)[i] : 0;
        }
    };
}
