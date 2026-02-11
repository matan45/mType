#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

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

        // Record a function entry — returns true if function just became hot
        bool recordEntry(const std::string& functionName);

        // Check if a function has reached the hot threshold
        bool isHot(const std::string& functionName) const;

        // Get the invocation count for a function
        uint32_t getInvocationCount(const std::string& functionName) const;

        // Get all functions that have been marked as hot
        const std::vector<std::string>& getHotFunctions() const;

        // Reset profiling data
        void reset();

        // Get/set the hot threshold
        uint32_t getHotThreshold() const { return hotThreshold; }
        void setHotThreshold(uint32_t threshold) { hotThreshold = threshold; }

    private:
        uint32_t hotThreshold;
        std::unordered_map<std::string, uint32_t> invocationCounts;
        std::vector<std::string> hotFunctions;
    };
}
