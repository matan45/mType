#pragma once

#include <string>

namespace vm::optimization
{
    /**
     * Per-pass enable flags for the bytecode post-processing pipeline.
     * Mirrors optimizer::OptimizationConfig — tests use noOptimization() and
     * flip on a single flag to exercise one pass in isolation.
     */
    class BytecodeOptimizationConfig
    {
    private:
        bool enableLoopOptimization = true;
        bool enablePeephole = true;
        bool enableTrivialSetterInlining = true;
        bool enableTrivialGetterInlining = true;
        bool enableLocalArrayFusion = true;

    public:
        BytecodeOptimizationConfig() = default;

        static BytecodeOptimizationConfig forRelease();
        static BytecodeOptimizationConfig noOptimization();

        bool isLoopOptimizationEnabled() const { return enableLoopOptimization; }
        bool isPeepholeEnabled() const { return enablePeephole; }
        bool isTrivialSetterInliningEnabled() const { return enableTrivialSetterInlining; }
        bool isTrivialGetterInliningEnabled() const { return enableTrivialGetterInlining; }
        bool isLocalArrayFusionEnabled() const { return enableLocalArrayFusion; }

        BytecodeOptimizationConfig& setLoopOptimization(bool enable) { enableLoopOptimization = enable; return *this; }
        BytecodeOptimizationConfig& setPeephole(bool enable) { enablePeephole = enable; return *this; }
        BytecodeOptimizationConfig& setTrivialSetterInlining(bool enable) { enableTrivialSetterInlining = enable; return *this; }
        BytecodeOptimizationConfig& setTrivialGetterInlining(bool enable) { enableTrivialGetterInlining = enable; return *this; }
        BytecodeOptimizationConfig& setLocalArrayFusion(bool enable) { enableLocalArrayFusion = enable; return *this; }

        // Generic by-name flag query — used by the pass manager so a future
        // pass can be gated without adding another isXEnabled() accessor.
        // Unknown names default to true (caller-provided passes are opt-out).
        bool isPassEnabled(const std::string& passName) const;
    };
}
