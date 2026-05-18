#pragma once

#include "../BytecodeOptimizationConfig.hpp"

namespace vm::optimization::base
{
    /**
     * Per-run context for bytecode passes. Carries the immutable config and
     * an "anything changed" flag the manager can read after each pass.
     * Mirrors optimizer::base::OptimizationContext but minus the symbol-use
     * tracking sets — bytecode passes don't need a side channel for
     * inter-pass symbol-use accounting (each pass walks the program directly).
     */
    class BytecodeOptimizationContext
    {
    private:
        const BytecodeOptimizationConfig& config;
        bool modified = false;

    public:
        explicit BytecodeOptimizationContext(const BytecodeOptimizationConfig& cfg)
            : config(cfg) {}

        const BytecodeOptimizationConfig& getConfig() const { return config; }

        void setModified(bool wasModified) { modified = wasModified; }
        bool wasModified() const { return modified; }
    };
}
