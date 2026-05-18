#pragma once

#include "../base/BytecodePass.hpp"

namespace vm::optimization::passes
{
    /**
     * Adapter wrapping vm::optimization::PeepholeOptimizer for the bytecode
     * pass pipeline. The underlying optimizer has its own pattern registry
     * and statistics; this adapter forwards the call and preserves the
     * historic behaviour where peephole failures are logged and the rest of
     * the post-AST pipeline continues (the swallow that used to live in
     * BytecodeCompiler.cpp).
     */
    class PeepholeOptimizationPass : public base::BytecodePass
    {
    public:
        PeepholeOptimizationPass();

        void optimize(bytecode::BytecodeProgram& program,
                      base::BytecodeOptimizationContext& context) override;

        std::string getDescription() const override
        {
            return "Peephole instruction-pattern rewrites (release-mode config)";
        }
    };
}
