#include "LoopProfiler.hpp"

namespace vm::jit
{
    const char* osrBailoutReasonName(OSRBailoutReason r)
    {
        switch (r)
        {
            case OSRBailoutReason::NONE:                    return "NONE";
            case OSRBailoutReason::LOOP_MARKERS_MISSING:    return "LOOP_MARKERS_MISSING";
            case OSRBailoutReason::SHARED_FRAME_REJECTION:  return "SHARED_FRAME_REJECTION";
            case OSRBailoutReason::NO_FUNCTION_FRAME:       return "NO_FUNCTION_FRAME";
            case OSRBailoutReason::OPERAND_STACK_NOT_EMPTY: return "OPERAND_STACK_NOT_EMPTY";
            case OSRBailoutReason::UNSUPPORTED_OPCODE:      return "UNSUPPORTED_OPCODE";
            case OSRBailoutReason::LOCAL_COUNT_EXCEEDED:    return "LOCAL_COUNT_EXCEEDED";
            case OSRBailoutReason::BAILOUT_OPCODE:          return "BAILOUT_OPCODE";
            case OSRBailoutReason::CODEGEN_FAILURE:         return "CODEGEN_FAILURE";
        }
        return "UNKNOWN";
    }

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

    void LoopProfiler::markFailed(const LoopId& loopId,
                                   OSRBailoutReason reason,
                                   uint8_t offendingOpcode)
    {
        auto& profile = profiles[loopId];
        profile.osrFailed = true;
        // Preserve a more-specific earlier reason if one was already recorded
        // and the new call is the generic CODEGEN_FAILURE fallback.
        if (profile.bailoutReason == OSRBailoutReason::NONE ||
            reason != OSRBailoutReason::CODEGEN_FAILURE)
        {
            profile.bailoutReason = reason;
            profile.offendingOpcode = offendingOpcode;
        }
    }

    void LoopProfiler::reset()
    {
        profiles.clear();
    }
}
