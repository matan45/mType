#pragma once

#include <memory>
#include "../vm/optimization/BytecodeOptimizationResult.hpp"

namespace vm::bytecode { class BytecodeProgram; }
namespace vm::optimization { class BytecodeOptimizer; }

namespace services
{
    /**
     * Service for applying bytecode-level post-processing passes.
     * Mirrors services::OptimizationService (which owns the AST-level
     * pipeline). The two services run at different points in the build:
     *   - OptimizationService:           AST in,  AST out  (pre-emission).
     *   - BytecodeOptimizationService:   Program in, Program mutated in
     *                                    place (post-emission).
     */
    class BytecodeOptimizationService
    {
    private:
        std::unique_ptr<vm::optimization::BytecodeOptimizer> optimizer;

    public:
        BytecodeOptimizationService();
        ~BytecodeOptimizationService();

        vm::optimization::BytecodeOptimizationResult
            optimize(vm::bytecode::BytecodeProgram& program);
    };
}
