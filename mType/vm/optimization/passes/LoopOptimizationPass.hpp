#pragma once

#include "../base/BytecodePass.hpp"

namespace vm::optimization::passes
{
    /**
     * Adapter wrapping vm::runtime::optimization::LoopOptimizer for the
     * bytecode pass pipeline. The underlying optimizer lives under
     * runtime/optimization/ for historical reasons; it is in fact a
     * compile-time pass and is invoked once after AST → bytecode emission.
     */
    class LoopOptimizationPass : public base::BytecodePass
    {
    public:
        LoopOptimizationPass();

        void optimize(bytecode::BytecodeProgram& program,
                      base::BytecodeOptimizationContext& context) override;

        std::string getDescription() const override
        {
            return "Loop hoisting and back-edge optimization "
                   "(LOOP_START / LOOP_END markers)";
        }
    };
}
