#include "JitFrameAnalysisTests.hpp"

#include "BytecodeOptimizationTestSuite.hpp"
#include "../../vm/bytecode/BytecodeProgram.hpp"
#include "../../vm/bytecode/OpCode.hpp"
#include "../../vm/jit/analysis/JitFrameAnalysis.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace tests::testSuite
{
    using services::ScriptAPI;
    using vm::bytecode::OpCode;

    namespace
    {
        void require(bool condition, const std::string& message)
        {
            if (!condition) throw std::runtime_error(message);
        }

        void emitPushInt(
            vm::bytecode::BytecodeProgram& program, int64_t value)
        {
            const size_t index = program.getConstantPool().addInteger(value);
            program.emit(OpCode::PUSH_INT, static_cast<uint64_t>(index));
        }
    }

    void registerJitFrameAnalysisTests(
        BytecodeOptimizationTestSuite& suite)
    {
        suite.addCallbackTest(
            "JIT frame analysis proves typed CFG joins and short-circuit edges",
            "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram joinedProgram;
                joinedProgram.emit(OpCode::PUSH_BOOL, 1);
                joinedProgram.emit(OpCode::JUMP_IF_FALSE, 6);
                emitPushInt(joinedProgram, 42);
                joinedProgram.emit(OpCode::STORE_LOCAL, 0);
                joinedProgram.emit(OpCode::POP);
                joinedProgram.emit(OpCode::JUMP, 9);
                joinedProgram.emit(OpCode::PUSH_BOOL, 0);
                joinedProgram.emit(OpCode::STORE_LOCAL, 0);
                joinedProgram.emit(OpCode::POP);
                joinedProgram.emit(OpCode::LOAD_LOCAL, 0);
                joinedProgram.emit(OpCode::RETURN_VALUE);

                vm::bytecode::BytecodeProgram::FunctionMetadata joined{};
                joined.name = "typedJoin";
                joined.mangledName = joined.name;
                joined.startOffset = 0;
                joined.instructionCount = joinedProgram.getInstructionCount();
                joined.localCount = 1;
                joined.returnType = "int";

                const auto joinedResult =
                    vm::jit::analysis::analyzeFunctionFrame(
                        joinedProgram, joined, nullptr, true);
                require(joinedResult.proven(),
                    "equal-depth typed CFG joins must be provable");
                require(joinedResult.baseOperandStackPeak == 1,
                    "typed join should require exactly one operand slot");
                require(joinedResult.typeWideningCount != 0,
                    "conflicting primitive types must widen at the join");

                vm::bytecode::BytecodeProgram shortCircuitProgram;
                shortCircuitProgram.emit(OpCode::PUSH_BOOL, 1);
                shortCircuitProgram.emit(OpCode::JUMP_IF_TRUE_OR_POP, 3);
                shortCircuitProgram.emit(OpCode::PUSH_BOOL, 0);
                shortCircuitProgram.emit(OpCode::RETURN_VALUE);

                vm::bytecode::BytecodeProgram::FunctionMetadata shortCircuit{};
                shortCircuit.name = "shortCircuit";
                shortCircuit.mangledName = shortCircuit.name;
                shortCircuit.startOffset = 0;
                shortCircuit.instructionCount =
                    shortCircuitProgram.getInstructionCount();
                shortCircuit.localCount = 1;
                shortCircuit.returnType = "bool";

                const auto shortCircuitResult =
                    vm::jit::analysis::analyzeFunctionFrame(
                        shortCircuitProgram, shortCircuit, nullptr, true);
                require(shortCircuitResult.proven()
                        && shortCircuitResult.baseOperandStackPeak == 1,
                    "short-circuit edges must merge at depth one");
            });

        suite.addCallbackTest(
            "JIT frame analysis rejects lexical emitter depth divergence", "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                program.emit(OpCode::PUSH_BOOL, 1);
                program.emit(OpCode::JUMP_IF_FALSE, 4);
                emitPushInt(program, 42);
                program.emit(OpCode::JUMP, 5);
                program.emit(OpCode::PUSH_BOOL, 0);
                program.emit(OpCode::RETURN_VALUE);

                vm::bytecode::BytecodeProgram::FunctionMetadata function{};
                function.name = "jumpOverValue";
                function.mangledName = function.name;
                function.startOffset = 0;
                function.instructionCount = program.getInstructionCount();
                function.localCount = 1;
                function.returnType = "int";

                const auto result = vm::jit::analysis::analyzeFunctionFrame(
                    program, function, nullptr, true);
                require(result.status == vm::jit::analysis::FrameAnalysisStatus::
                            EMITTER_STACK_MODEL_MISMATCH,
                    "jumping over a value-producing arm must fail emission proof");
                require(result.offendingOffset == 4,
                    "emitter divergence must identify the alternate-arm entry");

                const auto plan = vm::jit::analysis::makeFrameLayoutPlan(
                    result, 256, 96);
                require(!plan.valid && !plan.usedConservativeFallback,
                    "wrong-slot emission cannot be repaired by a wider frame");
            });

        suite.addCallbackTest(
            "JIT frame analysis rejects malformed joins and bounds unknown effects",
            "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                program.emit(OpCode::PUSH_BOOL, 1);
                program.emit(OpCode::JUMP_IF_FALSE, 3);
                emitPushInt(program, 7);
                program.emit(OpCode::RETURN);

                vm::bytecode::BytecodeProgram::FunctionMetadata function{};
                function.name = "mismatchedJoin";
                function.mangledName = function.name;
                function.startOffset = 0;
                function.instructionCount = program.getInstructionCount();
                function.localCount = 1;
                function.returnType = "void";

                const auto result = vm::jit::analysis::analyzeFunctionFrame(
                    program, function, nullptr, true);
                require(result.status == vm::jit::analysis::FrameAnalysisStatus::
                            STACK_DEPTH_MERGE_MISMATCH,
                    "different operand depths at a CFG join must fail closed");

                const auto plan = vm::jit::analysis::makeFrameLayoutPlan(
                    result, 256, 96);
                require(!plan.valid && !plan.usedConservativeFallback,
                    "a wider frame cannot repair a malformed CFG join");

                vm::jit::analysis::FrameAnalysisResult unsupported{};
                unsupported.status = vm::jit::analysis::FrameAnalysisStatus::
                    UNSUPPORTED_STACK_EFFECT;
                const auto fallback = vm::jit::analysis::makeFrameLayoutPlan(
                    unsupported, 256, 96);
                require(fallback.valid && fallback.usedConservativeFallback
                        && fallback.operandStackSlots == 256
                        && fallback.inlineLocalSlots == 96,
                    "unmodelled safe effects must retain historical frame caps");
            });

        suite.addCallbackTest(
            "JIT frame analysis reserves eligible inline locals", "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                emitPushInt(program, 9);
                program.emit(OpCode::RETURN_VALUE);

                vm::bytecode::BytecodeProgram::FunctionMetadata callee{};
                callee.name = "tinyCallee";
                callee.mangledName = callee.name;
                callee.startOffset = 0;
                callee.instructionCount = 2;
                callee.localCount = 7;
                callee.returnType = "int";
                program.registerFunction(callee.name, callee);

                const size_t calleeName =
                    program.getConstantPool().addString(callee.name);
                program.emit(OpCode::CALL,
                    static_cast<uint64_t>(calleeName), 0);
                program.emit(OpCode::RETURN_VALUE);

                vm::bytecode::BytecodeProgram::FunctionMetadata caller{};
                caller.name = "tinyCaller";
                caller.mangledName = caller.name;
                caller.startOffset = 2;
                caller.instructionCount = 2;
                caller.localCount = 1;
                caller.returnType = "int";
                program.registerFunction(caller.name, caller);

                const auto result = vm::jit::analysis::analyzeFunctionFrame(
                    program, caller, nullptr, true);
                require(result.proven() && result.inlineLocalSlots == 7,
                    "eligible plain calls must reserve the callee local window");

                const auto plan = vm::jit::analysis::makeFrameLayoutPlan(
                    result, 256, 96);
                require(plan.valid && !plan.usedConservativeFallback,
                    "a proven inline call should use a tight frame layout");
                require(plan.operandStackSlots == 8,
                    "small proven functions should use the minimum operand frame");
                require(plan.inlineLocalSlots == 9,
                    "inline-local layout must include the two-slot guard band");
            });

        suite.addCallbackTest(
            "JIT frame analysis skips oversized inline local windows", "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                emitPushInt(program, 3);
                program.emit(OpCode::RETURN_VALUE);

                vm::bytecode::BytecodeProgram::FunctionMetadata callee{};
                callee.name = "wideCallee";
                callee.mangledName = callee.name;
                callee.startOffset = 0;
                callee.instructionCount = 2;
                callee.localCount =
                    vm::jit::analysis::DEFAULT_INLINE_LOCAL_HARD_LIMIT + 1;
                callee.returnType = "int";
                program.registerFunction(callee.name, callee);

                const size_t calleeName =
                    program.getConstantPool().addString(callee.name);
                program.emit(OpCode::CALL,
                    static_cast<uint64_t>(calleeName), 0);
                program.emit(OpCode::RETURN_VALUE);

                vm::bytecode::BytecodeProgram::FunctionMetadata caller{};
                caller.name = "wideCaller";
                caller.mangledName = caller.name;
                caller.startOffset = 2;
                caller.instructionCount = 2;
                caller.localCount = 1;
                caller.returnType = "int";

                const auto callerResult =
                    vm::jit::analysis::analyzeFunctionFrame(
                        program, caller, nullptr, true);
                require(callerResult.proven()
                        && callerResult.inlineLocalSlots == 0,
                    "oversized candidates must remain generic calls");

                const auto calleeResult =
                    vm::jit::analysis::analyzeInlinedCalleeFrame(
                        program, callee, nullptr, true,
                        caller.mangledName, false, 1);
                require(calleeResult.status ==
                        vm::jit::analysis::FrameAnalysisStatus::
                            INLINE_FRAME_LIMIT_EXCEEDED,
                    "direct inline analysis must reject oversized locals before allocation");
            });
    }
}
