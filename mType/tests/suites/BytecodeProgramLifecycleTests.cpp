#include "BytecodeProgramLifecycleTests.hpp"

#include "BytecodeOptimizationTestSuite.hpp"
#include "../../vm/bytecode/BytecodeProgram.hpp"
#include "../../vm/bytecode/OpCode.hpp"

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tests::testSuite
{
    using Program = vm::bytecode::BytecodeProgram;
    using vm::bytecode::OpCode;

    namespace
    {
        void require(bool condition, const std::string& message)
        {
            if (!condition) throw std::runtime_error(message);
        }

        struct ExpectedInstruction
        {
            OpCode opcode;
            std::vector<uint64_t> operands;
        };

        void requireColdInstructions(
            const Program& program,
            const std::vector<ExpectedInstruction>& expected)
        {
            require(program.getInstructionCount() == expected.size(),
                "semantic clone changed the instruction count");
            for (size_t offset = 0; offset < expected.size(); ++offset)
            {
                const auto& actual = program.getInstruction(offset);
                require(actual.opcode == expected[offset].opcode,
                    "semantic clone retained a runtime-only opcode at offset "
                    + std::to_string(offset));
                require(actual.numOperands() == expected[offset].operands.size(),
                    "semantic clone changed operand count at offset "
                    + std::to_string(offset));
                for (size_t operand = 0;
                     operand < expected[offset].operands.size(); ++operand)
                {
                    require(actual.operandAt(operand)
                                == expected[offset].operands[operand],
                        "semantic clone changed an operand at offset "
                        + std::to_string(offset));
                }
                require(program.findCachedState(offset) == nullptr,
                    "semantic clone retained process-local cached state");
            }
        }

        std::vector<ExpectedInstruction> warmProgram(Program& program)
        {
            program.setTopLevelLocalCount(10);
            const size_t intIndex = program.getConstantPool().addInteger(9);
            const size_t nameIndex = program.getConstantPool().addString("member");
            std::vector<ExpectedInstruction> expected;

            auto emitRuntimeFusion = [&](OpCode firstOpcode, uint64_t firstOperand,
                                         OpCode secondOpcode, OpCode fusedOpcode,
                                         OpCode coldSecond) {
                const size_t first = program.getInstructionCount();
                program.emit(firstOpcode, firstOperand);
                if (secondOpcode == OpCode::CALL_METHOD)
                    program.emit(secondOpcode, nameIndex, 0);
                else
                    program.emit(secondOpcode, nameIndex);
                auto& firstInstruction = program.getMutableInstruction(first);
                firstInstruction.opcode = OpCode::NOP;
                firstInstruction.clearOperands();
                auto& fused = program.getMutableInstruction(first + 1);
                fused.opcode = fusedOpcode;
                program.getOrCreateCachedState(first + 1).fusedSlot =
                    static_cast<uint32_t>(firstOperand);
                expected.push_back({firstOpcode, {firstOperand}});
                expected.push_back({coldSecond,
                    secondOpcode == OpCode::CALL_METHOD
                        ? std::vector<uint64_t>{nameIndex, 0}
                        : std::vector<uint64_t>{nameIndex}});
            };

            program.emit(OpCode::PUSH_INT, intIndex);
            program.emit(OpCode::ADD_INT);
            program.getMutableInstruction(0).opcode = OpCode::NOP;
            program.getMutableInstruction(0).clearOperands();
            program.getMutableInstruction(1).opcode = OpCode::ADD_INT_CONST;
            program.getOrCreateCachedState(1).fusedSlot =
                static_cast<uint32_t>(intIndex);
            expected.push_back({OpCode::PUSH_INT, {intIndex}});
            expected.push_back({OpCode::ADD_INT, {}});

            emitRuntimeFusion(OpCode::LOAD_LOCAL, 7, OpCode::CALL_METHOD,
                OpCode::LOAD_LOCAL_CALL_CACHED, OpCode::CALL_METHOD);
            emitRuntimeFusion(OpCode::LOAD_LOCAL, 8, OpCode::CALL_METHOD,
                OpCode::LOAD_LOCAL_CALL_POLY_CACHED, OpCode::CALL_METHOD);
            emitRuntimeFusion(OpCode::LOAD_LOCAL, 9, OpCode::GET_FIELD,
                OpCode::LOAD_LOCAL_GET_FIELD_CACHED, OpCode::GET_FIELD);

            const std::vector<std::pair<OpCode, OpCode>> quickenings{
                {OpCode::CALL_METHOD_CACHED, OpCode::CALL_METHOD},
                {OpCode::CALL_METHOD_POLY_CACHED, OpCode::CALL_METHOD},
                {OpCode::GET_FIELD_CACHED, OpCode::GET_FIELD},
                {OpCode::SET_FIELD_CACHED, OpCode::SET_FIELD},
                {OpCode::LOAD_VAR_CACHED, OpCode::LOAD_VAR},
                {OpCode::STORE_VAR_CACHED, OpCode::STORE_VAR},
                {OpCode::LOAD_LOCAL_INT, OpCode::LOAD_LOCAL},
                {OpCode::LOAD_LOCAL_FLOAT, OpCode::LOAD_LOCAL},
                {OpCode::LOAD_LOCAL_BOOL, OpCode::LOAD_LOCAL},
                {OpCode::LOAD_LOCAL_BOXED_INST, OpCode::LOAD_LOCAL},
                {OpCode::STORE_LOCAL_INT, OpCode::STORE_LOCAL},
                {OpCode::STORE_LOCAL_FLOAT, OpCode::STORE_LOCAL},
                {OpCode::STORE_LOCAL_BOOL, OpCode::STORE_LOCAL},
                {OpCode::STORE_LOCAL_BOXED_INST, OpCode::STORE_LOCAL},
                {OpCode::BITWISE_AND_INT, OpCode::BITWISE_AND_OP},
                {OpCode::BITWISE_OR_INT, OpCode::BITWISE_OR_OP},
                {OpCode::BITWISE_XOR_INT, OpCode::BITWISE_XOR_OP},
                {OpCode::LEFT_SHIFT_INT, OpCode::LEFT_SHIFT_OP},
                {OpCode::RIGHT_SHIFT_INT, OpCode::RIGHT_SHIFT_OP},
                {OpCode::BITWISE_NOT_INT, OpCode::BITWISE_NOT_OP},
            };
            for (const auto& [runtimeOpcode, semanticOpcode] : quickenings)
            {
                const bool hasOperand = semanticOpcode == OpCode::CALL_METHOD
                    || semanticOpcode == OpCode::GET_FIELD
                    || semanticOpcode == OpCode::SET_FIELD
                    || semanticOpcode == OpCode::LOAD_VAR
                    || semanticOpcode == OpCode::STORE_VAR
                    || semanticOpcode == OpCode::LOAD_LOCAL
                    || semanticOpcode == OpCode::STORE_LOCAL;
                const bool isMethod = semanticOpcode == OpCode::CALL_METHOD;
                if (isMethod) program.emit(runtimeOpcode, nameIndex, 0);
                else if (hasOperand) program.emit(runtimeOpcode, nameIndex);
                else program.emit(runtimeOpcode);
                expected.push_back({semanticOpcode,
                    isMethod ? std::vector<uint64_t>{nameIndex, 0}
                    : hasOperand ? std::vector<uint64_t>{nameIndex}
                                 : std::vector<uint64_t>{}});
            }

            auto& stale = program.getOrCreateCachedState(8);
            stale.cachedProgram = &program;
            stale.cachedMethodProgram = &program;
            stale.polyPrograms[0] = &program;
            stale.cachedJitFnPtr = reinterpret_cast<void*>(uintptr_t{1});
            program.primitiveWrapperCache.resize(1);
            program.primitiveWrapperCache[0].resolved = true;
            program.objectConstructionCache[1].resolved = true;
            return expected;
        }
    }

    void registerBytecodeProgramLifecycleTests(
        BytecodeOptimizationTestSuite& suite)
    {
        suite.addCallbackTest(
            "Warmed bytecode copies and serialization use cold semantics", "",
            [](services::ScriptAPI&) {
                std::unique_ptr<Program> copied;
                std::unique_ptr<Program> assigned = std::make_unique<Program>();
                std::unique_ptr<Program> roundTripped;
                std::vector<ExpectedInstruction> expected;
                vm::bytecode::FunctionNameHandle functionHandle{};
                vm::bytecode::FunctionNameHandle extraHandle{};
                vm::bytecode::ProgramId sourceId{};

                {
                    Program source;
                    expected = warmProgram(source);
                    Program::FunctionMetadata function{};
                    function.name = "copyTarget";
                    function.mangledName = function.name;
                    function.startOffset = 0;
                    function.instructionCount = source.getInstructionCount();
                    function.localCount = 10;
                    function.returnType = "void";
                    source.registerFunction(function.name, function);
                    functionHandle = source.internFrameName(function.name);
                    extraHandle = source.internFrameName("runtime-only-frame");
                    sourceId = source.getProgramId();

                    copied = std::make_unique<Program>(source);
                    *assigned = source;

                    std::stringstream bytecode;
                    source.serialize(bytecode);
                    roundTripped = std::make_unique<Program>(
                        Program::deserialize(bytecode));
                }

                require(copied->getProgramId() != sourceId
                        && assigned->getProgramId() != sourceId,
                    "copy construction/assignment must allocate fresh identities");
                requireColdInstructions(*copied, expected);
                requireColdInstructions(*assigned, expected);
                requireColdInstructions(*roundTripped, expected);
                require(copied->primitiveWrapperCache.empty()
                        && copied->objectConstructionCache.empty(),
                    "semantic copy retained environment-bound resolution caches");
                require(copied->getFrameName(functionHandle) == "copyTarget"
                        && copied->getFrameName(extraHandle)
                            == "runtime-only-frame",
                    "semantic copy did not rebuild frame-name views");
                require(copied->getFunctionMeta(functionHandle)
                            == copied->getFunction("copyTarget"),
                    "semantic copy retained a source FunctionMetadata pointer");
            });

        suite.addCallbackTest(
            "Moving unbound bytecode rebinds self-referential cache pointers", "",
            [](services::ScriptAPI&) {
                Program source;
                source.emit(OpCode::CALL, 0, 0);
                auto& state = source.getOrCreateCachedState(0);
                state.cachedProgram = &source;
                state.cachedMethodProgram = &source;
                state.polyPrograms[0] = &source;
                Program::FunctionMetadata function{};
                function.name = "moveTarget";
                function.mangledName = function.name;
                function.startOffset = 0;
                function.instructionCount = 1;
                function.localCount = 0;
                function.returnType = "void";
                source.registerFunction(function.name, function);
                const auto functionHandle =
                    source.internFrameName(function.name);
                const auto identity = source.getProgramId();

                Program moved(std::move(source));
                const auto* movedState = moved.findCachedState(0);
                require(moved.getProgramId() == identity,
                    "move construction changed program identity");
                require(movedState && movedState->cachedProgram == &moved
                        && movedState->cachedMethodProgram == &moved
                        && movedState->polyPrograms[0] == &moved,
                    "move construction retained the old program address");
                require(moved.getFrameName(functionHandle) == "moveTarget"
                        && moved.getFunctionMeta(functionHandle)
                            == moved.getFunction("moveTarget"),
                    "move construction retained stale frame-name indexes");

                Program assigned;
                assigned = std::move(moved);
                const auto* assignedState = assigned.findCachedState(0);
                require(assigned.getProgramId() == identity,
                    "move assignment changed program identity");
                require(assignedState && assignedState->cachedProgram == &assigned
                        && assignedState->cachedMethodProgram == &assigned
                        && assignedState->polyPrograms[0] == &assigned,
                    "move assignment retained the old program address");
                require(assigned.getFrameName(functionHandle) == "moveTarget"
                        && assigned.getFunctionMeta(functionHandle)
                            == assigned.getFunction("moveTarget"),
                    "move assignment retained stale frame-name indexes");
            });

        suite.addCallbackTest(
            "Runtime-bound bytecode rejects relocation and replacement", "",
            [](services::ScriptAPI&) {
                Program boundSource;
                boundSource.emit(OpCode::NOP);
                const auto boundId = boundSource.getProgramId();
                boundSource.pinRuntimeAddress();

                bool moveConstructionRejected = false;
                try
                {
                    Program invalid(std::move(boundSource));
                    (void)invalid;
                }
                catch (const std::logic_error&)
                {
                    moveConstructionRejected = true;
                }
                require(moveConstructionRejected
                        && boundSource.getProgramId() == boundId
                        && boundSource.getInstructionCount() == 1,
                    "moving a runtime-bound source must fail before mutation");

                Program unboundDestination;
                const auto unboundDestinationId =
                    unboundDestination.getProgramId();
                bool boundSourceAssignmentRejected = false;
                try
                {
                    unboundDestination = std::move(boundSource);
                }
                catch (const std::logic_error&)
                {
                    boundSourceAssignmentRejected = true;
                }
                require(boundSourceAssignmentRejected
                        && unboundDestination.getProgramId()
                            == unboundDestinationId,
                    "move assignment must reject a runtime-bound source");

                Program pinnedDestination;
                pinnedDestination.emit(OpCode::NOP);
                const auto pinnedDestinationId =
                    pinnedDestination.getProgramId();
                pinnedDestination.pinRuntimeAddress();
                Program replacement;
                replacement.emit(OpCode::RETURN);

                bool destinationMoveRejected = false;
                try
                {
                    pinnedDestination = std::move(replacement);
                }
                catch (const std::logic_error&)
                {
                    destinationMoveRejected = true;
                }

                bool destinationCopyRejected = false;
                try
                {
                    pinnedDestination = boundSource;
                }
                catch (const std::logic_error&)
                {
                    destinationCopyRejected = true;
                }
                require(destinationMoveRejected && destinationCopyRejected
                        && pinnedDestination.getProgramId()
                            == pinnedDestinationId
                        && pinnedDestination.getInstruction(0).opcode
                            == OpCode::NOP,
                    "a runtime-bound destination must not be replaced");

                Program coldClone(boundSource);
                require(coldClone.getProgramId() != boundId
                        && !coldClone.isRuntimeAddressPinned(),
                    "copy construction must remain the cold-clone escape hatch");
            });
    }
}
