#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "JitIdentity.hpp"

namespace vm::jit
{
    struct LoopId
    {
        bytecode::ProgramId programId;
        size_t jumpBackOffset;

        bool operator==(const LoopId& other) const noexcept
        {
            return programId == other.programId
                && jumpBackOffset == other.jumpBackOffset;
        }
    };

    struct LoopIdHash
    {
        size_t operator()(const LoopId& id) const noexcept
        {
            return combineIdentityHash(
                bytecode::ProgramIdHash{}(id.programId),
                std::hash<size_t>{}(id.jumpBackOffset));
        }
    };

    // MYT-148 Phase 1: when a loop fails to OSR-compile, record WHICH gate in
    // the OSR pipeline tripped.
    enum class OSRBailoutReason : uint8_t
    {
        NONE = 0,                    // Profile has not attempted OSR yet
        LOOP_MARKERS_MISSING,        // No enclosing LOOP_START / LOOP_END
        SHARED_FRAME_REJECTION,      // Lambda-captured/shared frame
        NO_FUNCTION_FRAME,           // Missing frame metadata/local count
        OPERAND_STACK_NOT_EMPTY,     // Values remain above locals at back-edge
        UNSUPPORTED_OPCODE,          // canCompileLoopOSR rejected an opcode
        LOCAL_COUNT_EXCEEDED,        // Native-frame local limit exceeded
        CODEGEN_FAILURE,             // Emission failed
        FINALIZE_FAILURE             // AsmJit finalize/add failed
    };

    const char* osrBailoutReasonName(OSRBailoutReason r);

    struct LoopProfile
    {
        uint32_t iterationCount = 0;
        uint32_t effectiveThreshold = 0;
        OSRBailoutReason bailoutReason = OSRBailoutReason::NONE;
        uint8_t offendingOpcode = 0; // UNSUPPORTED_OPCODE / CODEGEN_FAILURE
        bool observed = false;
        bool osrAttempted = false;
        bool osrCompiled = false;
        bool osrFailed = false;
    };

    class LoopProfiler
    {
    public:
        using ProfileMap = std::unordered_map<LoopId, LoopProfile, LoopIdHash>;

        explicit LoopProfiler(uint32_t osrThreshold = 500);

        // Record a loop back-edge hit. A zero span preserves the configured
        // threshold exactly. The VM supplies an approximate bytecode span so
        // larger loops can tier modestly earlier based on interpreted work.
        bool recordIteration(const LoopId& loopId,
                             size_t approximateLoopSpan = 0);

        const LoopProfile* getProfile(const LoopId& loopId) const;
        LoopProfile& getOrCreateProfile(const LoopId& loopId,
                                        size_t approximateLoopSpan = 0);

        void markCompiled(const LoopId& loopId);
        void markFailed(const LoopId& loopId,
                        OSRBailoutReason reason = OSRBailoutReason::CODEGEN_FAILURE,
                        uint8_t offendingOpcode = 0);

        uint32_t getOsrThreshold() const { return osrThreshold; }
        void setOsrThreshold(uint32_t threshold) { osrThreshold = threshold; }
        uint32_t thresholdForLoopSpan(size_t approximateLoopSpan) const noexcept;

        // Statistics retain the historical map-shaped API. It is rebuilt on
        // demand from dense per-program slots, so the back-edge hot path never
        // updates a second associative representation.
        const ProfileMap& getProfiles() const;

        void reset();

    private:
        struct ProgramProfiles
        {
            std::vector<LoopProfile> byJumpBackOffset;
            std::vector<size_t> observedOffsets;
        };

        ProgramProfiles& selectProgram(bytecode::ProgramId programId);
        LoopProfile& ensureProfile(ProgramProfiles& programProfiles,
                                   size_t jumpBackOffset,
                                   size_t approximateLoopSpan);

        uint32_t osrThreshold;
        std::unordered_map<bytecode::ProgramId, std::unique_ptr<ProgramProfiles>,
                           bytecode::ProgramIdHash> programs;

        // Nearly all consecutive back-edges belong to the same active
        // BytecodeProgram. Cache its dense table so the common path avoids a
        // ProgramId hash lookup entirely.
        bytecode::ProgramId activeProgramId{};
        ProgramProfiles* activeProgram = nullptr;

        mutable ProfileMap profileSnapshot;
        size_t observedProfileCount = 0;
    };
}
