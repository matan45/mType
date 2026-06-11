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

    // MYT-148 Phase 1: when a loop fails to OSR-compile, record WHICH gate in
    // the OSR pipeline tripped. Needed to diagnose why --jit-stats reports
    // `OSR compiled = 0` across every benchmark loop; without it, we're
    // guessing which of several distinct failure sites is firing for a given
    // script.
    enum class OSRBailoutReason : uint8_t
    {
        NONE = 0,                    // Profile hasn't attempted OSR yet / reserved default
        LOOP_MARKERS_MISSING,        // captureState()/findLoopBoundaries(): no LOOP_START/LOOP_END
        SHARED_FRAME_REJECTION,      // captureState(): function has a shared (lambda-capture) frame
        NO_FUNCTION_FRAME,           // captureState(): callStack empty or function metadata missing → localCount = 0
        OPERAND_STACK_NOT_EMPTY,     // captureState(): values left on operand stack at back-edge
        UNSUPPORTED_OPCODE,          // canCompileLoopOSR(): opcode not in getSupportedOpcodes
        LOCAL_COUNT_EXCEEDED,        // compileLoopOSR(): localCount > MAX_LOCAL_COUNT
        CODEGEN_FAILURE,             // Generic compileFailed during emission
        FINALIZE_FAILURE             // finalizeAndStore(): asmjit finalize/add rejected the emitted IR
    };

    const char* osrBailoutReasonName(OSRBailoutReason r);

    struct LoopProfile
    {
        uint32_t iterationCount = 0;
        bool osrAttempted = false;
        bool osrCompiled = false;
        bool osrFailed = false;
        OSRBailoutReason bailoutReason = OSRBailoutReason::NONE;
        uint8_t offendingOpcode = 0;  // Populated for UNSUPPORTED_OPCODE / CODEGEN_FAILURE
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
        // MYT-148: extended to record the bailout reason + offending opcode.
        void markFailed(const LoopId& loopId,
                         OSRBailoutReason reason = OSRBailoutReason::CODEGEN_FAILURE,
                         uint8_t offendingOpcode = 0);

        uint32_t getOsrThreshold() const { return osrThreshold; }
        void setOsrThreshold(uint32_t threshold) { osrThreshold = threshold; }

        const std::unordered_map<LoopId, LoopProfile, LoopIdHash>& getProfiles() const { return profiles; }

        void reset();

    private:
        uint32_t osrThreshold;
        std::unordered_map<LoopId, LoopProfile, LoopIdHash> profiles;
    };
}
