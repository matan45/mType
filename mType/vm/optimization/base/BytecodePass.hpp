#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include "../../bytecode/BytecodeProgram.hpp"

namespace vm::optimization
{
    class BytecodeOptimizationResult;
}

namespace vm::optimization::base
{
    class BytecodeOptimizationContext;

    // Distinguishes analysis-only passes from rewrites. Mirrors
    // optimizer::base::PassType so the two pipelines speak the same language,
    // even though only TRANSFORMATION is used today.
    enum class PassType
    {
        ANALYSIS,
        TRANSFORMATION,
        VALIDATION
    };

    /**
     * Abstract base for bytecode-level post-processing passes.
     *
     * Mirrors optimizer::base::OptimizationPass but operates on
     * vm::bytecode::BytecodeProgram in place — bytecode passes use the
     * program's mutator API (getMutableInstruction / replaceInstructions /
     * updateAllJumpOffsets) directly rather than rebuilding a new program.
     */
    class BytecodePass
    {
    protected:
        std::string passName;
        PassType passType;
        bool enabled = true;

        size_t transformationCount = 0;
        std::chrono::milliseconds executionTime{0};

    public:
        BytecodePass(std::string name, PassType type);
        virtual ~BytecodePass() = default;

        virtual void optimize(bytecode::BytecodeProgram& program,
                              BytecodeOptimizationContext& context) = 0;

        virtual std::string getName() const { return passName; }
        virtual std::string getDescription() const = 0;
        PassType getType() const { return passType; }

        void setEnabled(bool enable) { enabled = enable; }
        bool isEnabled() const { return enabled; }

        size_t getTransformationCount() const { return transformationCount; }
        std::chrono::milliseconds getExecutionTime() const { return executionTime; }

        virtual void reportMetrics(BytecodeOptimizationResult& result) const;
        virtual void reset();

    protected:
        void recordTransformation() { ++transformationCount; }
        void setExecutionTime(std::chrono::milliseconds time) { executionTime = time; }
    };
}
