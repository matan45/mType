#include "BytecodeOptimizationTestSuite.hpp"

#include "../../vm/bytecode/BytecodeProgram.hpp"
#include "../../vm/bytecode/OpCode.hpp"
#include "../../vm/optimization/BytecodeOptimizer.hpp"
#include "../../vm/optimization/BytecodeOptimizationConfig.hpp"
#include "../../vm/optimization/passes/LocalArrayFusionPass.hpp"
#include "../../vm/optimization/passes/PeepholeOptimizationPass.hpp"
#include "../../vm/optimization/base/BytecodeOptimizationContext.hpp"

#include <stdexcept>
#include <string>

namespace tests::testSuite
{
    using namespace testFramework;
    using services::ScriptAPI;
    using vm::bytecode::OpCode;

    namespace
    {
        void require(bool cond, const std::string& msg)
        {
            if (!cond) throw std::runtime_error(msg);
        }
    }

    void BytecodeOptimizationTestSuite::setupTests()
    {
        // LocalArrayFusionPass · Pattern 4 (MYT-303)
        //
        // ARRAY_GET followed by STORE_LOCAL is rewritten to ARRAY_GET_ALIAS
        // followed by STORE_LOCAL. The whole-program scan runs without
        // needing a registered function — the cleanest single-pass test.
        addCallbackTest("LocalArrayFusion rewrites ARRAY_GET+STORE_LOCAL to ARRAY_GET_ALIAS",
            "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                program.emit(OpCode::ARRAY_GET);
                program.emit(OpCode::STORE_LOCAL, 0);

                vm::optimization::BytecodeOptimizationConfig cfg =
                    vm::optimization::BytecodeOptimizationConfig::noOptimization();
                cfg.setLocalArrayFusion(true);
                vm::optimization::base::BytecodeOptimizationContext ctx(cfg);

                vm::optimization::passes::LocalArrayFusionPass pass;
                pass.optimize(program, ctx);

                require(program.getInstruction(0).opcode == OpCode::ARRAY_GET_ALIAS,
                    "ARRAY_GET should be rewritten to ARRAY_GET_ALIAS");
                require(program.getInstruction(1).opcode == OpCode::STORE_LOCAL,
                    "STORE_LOCAL should be unchanged");
                require(pass.getTransformationCount() == 1,
                    "exactly one transformation should be recorded");
            });

        // LocalArrayFusionPass · negative
        //
        // ARRAY_GET followed by anything other than STORE_LOCAL must NOT
        // be rewritten — the optimizer is only valid when the result is
        // about to be aliased into a named local.
        addCallbackTest("LocalArrayFusion leaves ARRAY_GET alone when not aliased",
            "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                program.emit(OpCode::ARRAY_GET);
                program.emit(OpCode::POP);

                vm::optimization::BytecodeOptimizationConfig cfg =
                    vm::optimization::BytecodeOptimizationConfig::noOptimization();
                cfg.setLocalArrayFusion(true);
                vm::optimization::base::BytecodeOptimizationContext ctx(cfg);

                vm::optimization::passes::LocalArrayFusionPass pass;
                pass.optimize(program, ctx);

                require(program.getInstruction(0).opcode == OpCode::ARRAY_GET,
                    "ARRAY_GET should remain ARRAY_GET when the next op is not STORE_LOCAL");
                require(pass.getTransformationCount() == 0,
                    "no transformations should be recorded");
            });

        // BytecodeOptimizer · pipeline gating
        //
        // Constructed with noOptimization(), the optimizer registers zero
        // passes. The pipeline runs cleanly on any program and the result
        // metrics show zero pass metrics.
        addCallbackTest("BytecodeOptimizer with noOptimization runs zero passes",
            "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                program.emit(OpCode::PUSH_NULL);
                program.emit(OpCode::HALT);

                vm::optimization::BytecodeOptimizer optimizer(
                    vm::optimization::BytecodeOptimizationConfig::noOptimization());
                auto result = optimizer.optimize(program);

                require(result.getPassMetrics().empty(),
                    "no passes should run when every flag is disabled");
                require(program.getInstructionCount() == 2,
                    "program should be unchanged");
                require(program.getInstruction(0).opcode == OpCode::PUSH_NULL,
                    "first instruction preserved");
            });

        // BytecodeOptimizer · selective enable
        //
        // Enable just LocalArrayFusion via the config and confirm only
        // that pass appears in the result metrics.
        addCallbackTest("BytecodeOptimizer with only LocalArrayFusion enabled runs one pass",
            "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                program.emit(OpCode::ARRAY_GET);
                program.emit(OpCode::STORE_LOCAL, 0);

                auto cfg = vm::optimization::BytecodeOptimizationConfig::noOptimization();
                cfg.setLocalArrayFusion(true);
                vm::optimization::BytecodeOptimizer optimizer(cfg);
                auto result = optimizer.optimize(program);

                require(result.getPassMetrics().size() == 1,
                    "exactly one pass metric expected");
                require(result.getPassMetrics()[0].passName == "LocalArrayFusion",
                    "the one metric should be LocalArrayFusion");
                require(program.getInstruction(0).opcode == OpCode::ARRAY_GET_ALIAS,
                    "the gated pass should still rewrite the instruction");
            });
    }
}
