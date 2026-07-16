#include "LoopProfiler.hpp"

#include <algorithm>
#include <limits>

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
            case OSRBailoutReason::CODEGEN_FAILURE:         return "CODEGEN_FAILURE";
            case OSRBailoutReason::FINALIZE_FAILURE:        return "FINALIZE_FAILURE";
        }
        return "UNKNOWN";
    }

    LoopProfiler::LoopProfiler(uint32_t osrThreshold)
        : osrThreshold(osrThreshold)
    {
    }

    uint32_t LoopProfiler::thresholdForLoopSpan(
        size_t approximateLoopSpan) const noexcept
    {
        // Keep existing behaviour for missing/small spans. Larger loops spend
        // more interpreter work per back-edge, so discount at most 25% from
        // the base threshold. The cap is intentionally conservative: loop
        // size is only a proxy for both execution cost and compile cost.
        if (approximateLoopSpan == 0 || approximateLoopSpan <= 16
            || osrThreshold == 0)
        {
            return osrThreshold;
        }

        const uint64_t base = osrThreshold;
        const uint64_t maxDiscount = base / 4;
        const uint64_t extraBytecodes = approximateLoopSpan - 16;
        const uint64_t discount = extraBytecodes >= 64
            ? maxDiscount
            : std::min<uint64_t>((extraBytecodes * base) / 256,
                                 maxDiscount);
        return static_cast<uint32_t>(base - discount);
    }

    LoopProfiler::ProgramProfiles& LoopProfiler::selectProgram(
        bytecode::ProgramId programId)
    {
        if (activeProgram && activeProgramId == programId)
        {
            return *activeProgram;
        }

        auto [it, inserted] = programs.try_emplace(programId);
        (void)inserted;
        if (!it->second) it->second = std::make_unique<ProgramProfiles>();
        activeProgramId = programId;
        activeProgram = it->second.get();
        return *activeProgram;
    }

    LoopProfile& LoopProfiler::ensureProfile(
        ProgramProfiles& programProfiles,
        size_t jumpBackOffset,
        size_t approximateLoopSpan)
    {
        if (jumpBackOffset >= programProfiles.byJumpBackOffset.size())
        {
            programProfiles.byJumpBackOffset.resize(jumpBackOffset + 1);
        }

        auto& profile = programProfiles.byJumpBackOffset[jumpBackOffset];
        if (!profile.observed)
        {
            profile.observed = true;
            profile.effectiveThreshold =
                thresholdForLoopSpan(approximateLoopSpan);
            programProfiles.observedOffsets.push_back(jumpBackOffset);
            ++observedProfileCount;
        }
        return profile;
    }

    bool LoopProfiler::recordIteration(const LoopId& loopId,
                                       size_t approximateLoopSpan)
    {
        auto& programProfiles = selectProgram(loopId.programId);
        auto& profile = ensureProfile(programProfiles,
                                      loopId.jumpBackOffset,
                                      approximateLoopSpan);

        if (profile.osrAttempted || profile.osrFailed)
        {
            return false;
        }

        if (profile.iterationCount != std::numeric_limits<uint32_t>::max())
        {
            ++profile.iterationCount;
        }

        if (profile.iterationCount == profile.effectiveThreshold)
        {
            profile.osrAttempted = true;
            return true;
        }

        return false;
    }

    const LoopProfile* LoopProfiler::getProfile(const LoopId& loopId) const
    {
        const ProgramProfiles* programProfiles = nullptr;
        if (activeProgram && activeProgramId == loopId.programId)
        {
            programProfiles = activeProgram;
        }
        else
        {
            auto programIt = programs.find(loopId.programId);
            if (programIt == programs.end()) return nullptr;
            programProfiles = programIt->second.get();
        }

        if (loopId.jumpBackOffset >= programProfiles->byJumpBackOffset.size())
        {
            return nullptr;
        }
        const auto& profile =
            programProfiles->byJumpBackOffset[loopId.jumpBackOffset];
        return profile.observed ? &profile : nullptr;
    }

    LoopProfile& LoopProfiler::getOrCreateProfile(
        const LoopId& loopId,
        size_t approximateLoopSpan)
    {
        auto& programProfiles = selectProgram(loopId.programId);
        return ensureProfile(programProfiles,
                             loopId.jumpBackOffset,
                             approximateLoopSpan);
    }

    void LoopProfiler::markCompiled(const LoopId& loopId)
    {
        auto& profile = getOrCreateProfile(loopId);
        profile.osrCompiled = true;
    }

    void LoopProfiler::markFailed(const LoopId& loopId,
                                  OSRBailoutReason reason,
                                  uint8_t offendingOpcode)
    {
        auto& profile = getOrCreateProfile(loopId);
        profile.osrFailed = true;
        if (profile.bailoutReason == OSRBailoutReason::NONE
            || reason != OSRBailoutReason::CODEGEN_FAILURE)
        {
            profile.bailoutReason = reason;
            profile.offendingOpcode = offendingOpcode;
        }
    }

    const LoopProfiler::ProfileMap& LoopProfiler::getProfiles() const
    {
        profileSnapshot.clear();
        profileSnapshot.reserve(observedProfileCount);
        for (const auto& [programId, programProfilesPtr] : programs)
        {
            const auto& programProfiles = *programProfilesPtr;
            for (size_t offset : programProfiles.observedOffsets)
            {
                profileSnapshot.emplace(
                    LoopId{programId, offset},
                    programProfiles.byJumpBackOffset[offset]);
            }
        }
        return profileSnapshot;
    }

    void LoopProfiler::reset()
    {
        programs.clear();
        activeProgramId = {};
        activeProgram = nullptr;
        profileSnapshot.clear();
        observedProfileCount = 0;
    }
}
