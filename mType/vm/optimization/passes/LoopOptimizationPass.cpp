#include "LoopOptimizationPass.hpp"
#include "../base/BytecodeOptimizationContext.hpp"
#include "../../runtime/optimization/LoopOptimizer.hpp"

namespace vm::optimization::passes
{
    LoopOptimizationPass::LoopOptimizationPass()
        : BytecodePass("LoopOptimization", base::PassType::TRANSFORMATION)
    {
    }

    void LoopOptimizationPass::optimize(bytecode::BytecodeProgram& program,
                                        base::BytecodeOptimizationContext& context)
    {
        vm::runtime::optimization::LoopOptimizer loopOptimizer(program);
        loopOptimizer.optimize();
        // LoopOptimizer doesn't expose a per-run mutation count today;
        // record a single "ran" transformation so the metrics row shows up.
        recordTransformation();
        context.setModified(true);
    }
}
