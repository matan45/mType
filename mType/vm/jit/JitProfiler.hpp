#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "JitIdentity.hpp"

namespace vm::jit
{
    /**
     * Tracks function invocation counts and identifies hot functions
     * for JIT compilation. Activated via PROFILE_ENTER/PROFILE_EXIT opcodes.
     */
    class JitProfiler
    {
    public:
        explicit JitProfiler(uint32_t hotThreshold = 100);

        // Record a function entry - returns true if function just became hot.
        // A zero body size preserves the configured threshold exactly. This
        // keeps custom-threshold callers source-compatible while allowing the
        // VM to tier larger, higher-work invocations modestly earlier.
        bool recordEntry(bytecode::ProgramId programId,
                         std::string_view functionName,
                         size_t bytecodeBodySize = 0);

        // Check if a function has reached its effective hot threshold.
        bool isHot(const FunctionId& function) const;

        uint32_t getInvocationCount(const FunctionId& function) const;
        uint32_t getEffectiveThreshold(const FunctionId& function,
                                       size_t bytecodeBodySize = 0) const;

        // Public for telemetry and focused policy tests. A zero size returns
        // the configured base threshold.
        uint32_t thresholdForBodySize(size_t bytecodeBodySize) const noexcept;

        const std::vector<FunctionId>& getHotFunctions() const;
        void reset();

        uint32_t getHotThreshold() const { return hotThreshold; }
        void setHotThreshold(uint32_t threshold) { hotThreshold = threshold; }

    private:
        struct FunctionProfile
        {
            uint32_t invocationCount = 0;
            uint32_t effectiveThreshold = 0;
        };

        uint32_t hotThreshold;
        std::unordered_map<FunctionId, FunctionProfile,
                           FunctionIdHash, FunctionIdEqual> invocationProfiles;
        std::vector<FunctionId> hotFunctions;
    };
}
