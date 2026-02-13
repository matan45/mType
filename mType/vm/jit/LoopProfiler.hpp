#pragma once
#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <functional>

namespace vm::jit
{
    struct LoopId
    {
        size_t jumpBackOffset;

        bool operator==(const LoopId& other) const
        {
            return jumpBackOffset == other.jumpBackOffset;
        }
    };

    struct LoopIdHash
    {
        size_t operator()(const LoopId& id) const
        {
            return std::hash<size_t>{}(id.jumpBackOffset);
        }
    };

    struct LoopProfile
    {
        uint32_t iterationCount = 0;
        bool osrAttempted = false;
        bool osrCompiled = false;
        bool osrFailed = false;
    };

    class LoopProfiler
    {
    public:
        explicit LoopProfiler(uint32_t osrThreshold = 500);

        // Record a loop back-edge hit. Returns true on exact threshold crossing.
        bool recordIteration(const LoopId& loopId);

        const LoopProfile* getProfile(const LoopId& loopId) const;
        LoopProfile& getOrCreateProfile(const LoopId& loopId);

        void markCompiled(const LoopId& loopId);
        void markFailed(const LoopId& loopId);

        uint32_t getOsrThreshold() const { return osrThreshold; }
        void setOsrThreshold(uint32_t threshold) { osrThreshold = threshold; }

        void reset();

    private:
        uint32_t osrThreshold;
        std::unordered_map<LoopId, LoopProfile, LoopIdHash> profiles;
    };
}
