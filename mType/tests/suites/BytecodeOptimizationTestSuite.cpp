#include "BytecodeOptimizationTestSuite.hpp"

#include "../../vm/bytecode/BytecodeProgram.hpp"
#include "../../vm/bytecode/OpCode.hpp"
#include "../../vm/optimization/BytecodeOptimizer.hpp"
#include "../../vm/optimization/BytecodeOptimizationConfig.hpp"
#include "../../vm/optimization/passes/LocalArrayFusionPass.hpp"
#include "../../vm/optimization/passes/PeepholeOptimizationPass.hpp"
#include "../../vm/optimization/base/BytecodeOptimizationContext.hpp"

#include <cstdint>
#include <limits>
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

        void optimizePeephole(vm::bytecode::BytecodeProgram& program)
        {
            auto cfg = vm::optimization::BytecodeOptimizationConfig::noOptimization();
            cfg.setPeephole(true);
            vm::optimization::BytecodeOptimizer optimizer(cfg);
            optimizer.optimize(program);
        }

        void emitPushInt(vm::bytecode::BytecodeProgram& program, int64_t value)
        {
            const size_t index = program.getConstantPool().addInteger(value);
            program.emit(OpCode::PUSH_INT, static_cast<uint64_t>(index));
        }

        void requireSinglePushBool(const vm::bytecode::BytecodeProgram& program,
                                   bool expected,
                                   const std::string& context)
        {
            require(program.getInstructionCount() == 1,
                    context + ": expected one folded instruction");
            const auto& instr = program.getInstruction(0);
            require(instr.opcode == OpCode::PUSH_BOOL,
                    context + ": expected PUSH_BOOL");
            require(instr.operandCount == 1,
                    context + ": expected one inline bool operand");
            require(instr.inlineOperands[0] == (expected ? 1 : 0),
                    context + ": PUSH_BOOL operand must be inline 0/1");
        }

        void requireSinglePushInt(const vm::bytecode::BytecodeProgram& program,
                                  int64_t expected,
                                  const std::string& context)
        {
            require(program.getInstructionCount() == 1,
                    context + ": expected one folded instruction");
            const auto& instr = program.getInstruction(0);
            require(instr.opcode == OpCode::PUSH_INT,
                    context + ": expected PUSH_INT");
            require(instr.operandCount == 1,
                    context + ": expected one integer constant operand");
            int64_t actual = program.getConstantPool().getInteger(
                static_cast<size_t>(instr.inlineOperands[0]));
            require(actual == expected,
                    context + ": folded integer value mismatch");
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

        // Peephole ConstantFoldingPattern · bool operand convention (MYT-386)
        //
        // PUSH_BOOL is an inline-immediate opcode: operand 0 means false,
        // operand 1 means true. It must not carry an integer constant-pool
        // index, or folded false conditions can become truthy at runtime.
        addCallbackTest("Peephole folds false int comparison to inline PUSH_BOOL 0",
            "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                emitPushInt(program, 1);
                emitPushInt(program, 2);
                program.emit(OpCode::EQ_INT);

                optimizePeephole(program);

                requireSinglePushBool(program, false, "false int comparison");
            });

        addCallbackTest("Peephole folds true int comparison to inline PUSH_BOOL 1",
            "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                emitPushInt(program, 1);
                emitPushInt(program, 1);
                program.emit(OpCode::EQ_INT);

                optimizePeephole(program);

                requireSinglePushBool(program, true, "true int comparison");
            });

        addCallbackTest("Peephole folds boolean NOT to inline PUSH_BOOL",
            "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                program.emit(OpCode::PUSH_BOOL, 1);
                program.emit(OpCode::NOT);

                optimizePeephole(program);

                requireSinglePushBool(program, false, "boolean NOT");
            });

        addCallbackTest("Peephole folds boolean OR to inline PUSH_BOOL",
            "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                program.emit(OpCode::PUSH_BOOL, 0);
                program.emit(OpCode::PUSH_BOOL, 0);
                program.emit(OpCode::OR);

                optimizePeephole(program);

                requireSinglePushBool(program, false, "boolean OR");
            });

        addCallbackTest("Peephole folds INT64_MIN divided by -1 without host UB",
            "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                emitPushInt(program, std::numeric_limits<int64_t>::min());
                emitPushInt(program, -1);
                program.emit(OpCode::DIV_INT);

                optimizePeephole(program);

                requireSinglePushInt(program, std::numeric_limits<int64_t>::min(),
                    "INT64_MIN / -1");
            });

        addCallbackTest("Peephole folds INT64_MIN modulo -1 without host UB",
            "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                emitPushInt(program, std::numeric_limits<int64_t>::min());
                emitPushInt(program, -1);
                program.emit(OpCode::MOD);

                optimizePeephole(program);

                requireSinglePushInt(program, 0, "INT64_MIN % -1");
            });

        addCallbackTest("Peephole does not fold invalid boolean NEG",
            "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                program.emit(OpCode::PUSH_BOOL, 1);
                program.emit(OpCode::NEG);

                optimizePeephole(program);

                require(program.getInstructionCount() == 2,
                    "PUSH_BOOL + NEG should not be folded");
                require(program.getInstruction(0).opcode == OpCode::PUSH_BOOL,
                    "first instruction should remain PUSH_BOOL");
                require(program.getInstruction(0).inlineOperands[0] == 1,
                    "PUSH_BOOL operand should remain unchanged");
                require(program.getInstruction(1).opcode == OpCode::NEG,
                    "second instruction should remain NEG");
            });
    }
}
