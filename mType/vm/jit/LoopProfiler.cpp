#include "LoopProfiler.hpp"

namespace vm::jit
{
    LoopProfiler::LoopProfiler(uint32_t osrThreshold)
        : osrThreshold(osrThreshold)
    {}

    bool LoopProfiler::recordIteration(const LoopId& loopId)
    {
        auto& profile = profiles[loopId];

        if (profile.osrAttempted || profile.osrFailed)
        {
            return false;
        }

        profile.iterationCount++;

        if (profile.iterationCount == osrThreshold)
        {
            profile.osrAttempted = true;
            return true;
        }

        return false;
    }

    const LoopProfile* LoopProfiler::getProfile(const LoopId& loopId) const
    {
        auto it = profiles.find(loopId);
        if (it != profiles.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    LoopProfile& LoopProfiler::getOrCreateProfile(const LoopId& loopId)
    {
        return profiles[loopId];
    }

    void LoopProfiler::markCompiled(const LoopId& loopId)
    {
        auto& profile = profiles[loopId];
        profile.osrCompiled = true;
    }

    void LoopProfiler::markFailed(const LoopId& loopId)
    {
        auto& profile = profiles[loopId];
        profile.osrFailed = true;
    }

    void LoopProfiler::reset()
    {
        profiles.clear();
    }
}
