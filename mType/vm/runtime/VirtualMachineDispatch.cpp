#include "VirtualMachine.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include "executors/ArithmeticExecutor.hpp"
#include "executors/BitwiseExecutor.hpp"
#include "executors/ComparisonExecutor.hpp"
#include "executors/ControlFlowExecutor.hpp"
#include "executors/FunctionExecutor.hpp"
#include "executors/InlineCacheExecutor.hpp"
#include "executors/LogicalExecutor.hpp"
#include "executors/ObjectExecutor.hpp"
#include "executors/StackOperationsExecutor.hpp"
#include "executors/VariableExecutor.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../value/ValueShim.hpp"
#include "../../value/AsyncPromiseValue.hpp"
#include "../../value/PrimitiveTypeTag.hpp"

namespace vm::runtime
{
    void VirtualMachine::executeInstruction(const bytecode::BytecodeProgram::Instruction& instr)
    {
        using OpCode = bytecode::OpCode;
        const auto* activeProgram = executionCtx ? executionCtx->program : program;

        switch (instr.opcode)
        {
        // Stack operations
        case OpCode::PUSH_INT: stackOpsExecutor->handlePushInt(instr);
            break;
        case OpCode::PUSH_FLOAT: stackOpsExecutor->handlePushFloat(instr);
            break;
        case OpCode::PUSH_STRING: stackOpsExecutor->handlePushString(instr);
            break;
        case OpCode::PUSH_BOOL: stackOpsExecutor->handlePushBool(instr);
            break;
        case OpCode::PUSH_NULL: stackOpsExecutor->handlePushNull();
            break;
        case OpCode::POP: stackOpsExecutor->handlePop();
            break;
        case OpCode::DUP: stackOpsExecutor->handleDup();
            break;
        case OpCode::SWAP: stackOpsExecutor->handleSwap();
            break;

        // Arithmetic
        case OpCode::ADD:
            trySpecializeArithmetic(instr, OpCode::ADD_INT, OpCode::ADD_FLOAT);
            arithmeticExecutor->handleAdd();
            break;
        case OpCode::STRING_BUILD:
            arithmeticExecutor->handleStringBuild(static_cast<size_t>(instr.inlineOperands[0]));
            break;
        case OpCode::SUB:
            trySpecializeArithmetic(instr, OpCode::SUB_INT, OpCode::SUB_FLOAT);
            arithmeticExecutor->handleSub();
            break;
        case OpCode::MUL:
            trySpecializeArithmetic(instr, OpCode::MUL_INT, OpCode::MUL_FLOAT);
            arithmeticExecutor->handleMul();
            break;
        case OpCode::DIV:
            trySpecializeArithmetic(instr, OpCode::DIV_INT, OpCode::DIV_FLOAT);
            arithmeticExecutor->handleDiv();
            break;
        case OpCode::MOD: arithmeticExecutor->handleMod();
            break;
        case OpCode::NEG: arithmeticExecutor->handleNeg();
            break;
        case OpCode::INC: arithmeticExecutor->handleInc();
            break;
        case OpCode::DEC: arithmeticExecutor->handleDec();
            break;
        case OpCode::ADD_INT: arithmeticExecutor->handleAddInt();
            break;
        case OpCode::ADD_INT_CONST:
        {
            // MYT-198: fused PUSH_INT + ADD_INT. Never emitted by the compiler;
            // only reachable via runtime promotion inside trySpecializeArithmetic.
            // MYT-201: fused state lives on the side table — one lookup here
            // hands the reference to the handler. If the entry is somehow
            // missing (deserialization corruption, edge in the deopt cycle),
            // throw rather than assert — in release assert is a no-op and
            // the following *state deref would segfault with no diagnostic.
            const auto* state = activeProgram->findCachedState(instructionPointer);
            if (!state)
                throw errors::RuntimeException("ADD_INT_CONST dispatched without side-table entry");
            arithmeticExecutor->handleAddIntConst(instr, *state);
            break;
        }
        case OpCode::SUB_INT: arithmeticExecutor->handleSubInt();
            break;
        case OpCode::MUL_INT: arithmeticExecutor->handleMulInt();
            break;
        case OpCode::DIV_INT: arithmeticExecutor->handleDivInt();
            break;
        case OpCode::ADD_FLOAT: arithmeticExecutor->handleAddFloat();
            break;
        case OpCode::SUB_FLOAT: arithmeticExecutor->handleSubFloat();
            break;
        case OpCode::MUL_FLOAT: arithmeticExecutor->handleMulFloat();
            break;
        case OpCode::DIV_FLOAT: arithmeticExecutor->handleDivFloat();
            break;

        // Comparison
        case OpCode::EQ: comparisonExecutor->handleEq();
            break;
        case OpCode::NE: comparisonExecutor->handleNe();
            break;
        case OpCode::LT: comparisonExecutor->handleLt();
            break;
        case OpCode::GT: comparisonExecutor->handleGt();
            break;
        case OpCode::LE: comparisonExecutor->handleLe();
            break;
        case OpCode::GE: comparisonExecutor->handleGe();
            break;

        // Logical
        case OpCode::AND: logicalExecutor->handleAnd();
            break;
        case OpCode::OR: logicalExecutor->handleOr();
            break;
        case OpCode::NOT: logicalExecutor->handleNot();
            break;

        // Bitwise
        case OpCode::BITWISE_AND_OP:
            trySpecializeBitwise(instr, OpCode::BITWISE_AND_INT);
            bitwiseExecutor->handleBitwiseAnd();
            break;
        case OpCode::BITWISE_OR_OP:
            trySpecializeBitwise(instr, OpCode::BITWISE_OR_INT);
            bitwiseExecutor->handleBitwiseOr();
            break;
        case OpCode::BITWISE_XOR_OP:
            trySpecializeBitwise(instr, OpCode::BITWISE_XOR_INT);
            bitwiseExecutor->handleBitwiseXor();
            break;
        case OpCode::LEFT_SHIFT_OP:
            trySpecializeBitwise(instr, OpCode::LEFT_SHIFT_INT);
            bitwiseExecutor->handleLeftShift();
            break;
        case OpCode::RIGHT_SHIFT_OP:
            trySpecializeBitwise(instr, OpCode::RIGHT_SHIFT_INT);
            bitwiseExecutor->handleRightShift();
            break;
        case OpCode::BITWISE_NOT_OP:
            trySpecializeBitwiseUnary(instr, OpCode::BITWISE_NOT_INT);
            bitwiseExecutor->handleBitwiseNot();
            break;
        case OpCode::BITWISE_AND_INT: bitwiseExecutor->handleBitwiseAndInt();
            break;
        case OpCode::BITWISE_OR_INT: bitwiseExecutor->handleBitwiseOrInt();
            break;
        case OpCode::BITWISE_XOR_INT: bitwiseExecutor->handleBitwiseXorInt();
            break;
        case OpCode::LEFT_SHIFT_INT: bitwiseExecutor->handleLeftShiftInt();
            break;
        case OpCode::RIGHT_SHIFT_INT: bitwiseExecutor->handleRightShiftInt();
            break;
        case OpCode::BITWISE_NOT_INT: bitwiseExecutor->handleBitwiseNotInt();
            break;

        // Variables
        case OpCode::LOAD_VAR: variableExecutor->handleLoadVar(instr);
            break;
        case OpCode::STORE_VAR: variableExecutor->handleStoreVar(instr);
            break;
        case OpCode::LOAD_VAR_CACHED:
        {
            // MYT-204: promoted from LOAD_VAR after the global-resolution
            // path stabilised. Fall back to the generic handler if reached
            // without a side-table entry.
            const auto* state = activeProgram->findCachedState(instructionPointer);
            if (!state)
            {
                variableExecutor->handleLoadVar(instr);
                break;
            }
            variableExecutor->handleLoadVarCached(instr, *state);
            break;
        }
        case OpCode::STORE_VAR_CACHED:
        {
            // MYT-204: see LOAD_VAR_CACHED above.
            const auto* state = activeProgram->findCachedState(instructionPointer);
            if (!state)
            {
                variableExecutor->handleStoreVar(instr);
                break;
            }
            variableExecutor->handleStoreVarCached(instr, *state);
            break;
        }
        case OpCode::DECLARE_VAR: variableExecutor->handleDeclareVar(instr);
            break;
        case OpCode::LOAD_LOCAL: variableExecutor->handleLoadLocal(instr);
            break;
        case OpCode::STORE_LOCAL: variableExecutor->handleStoreLocal(instr);
            break;
        // MYT-199: type-quickened LOAD_LOCAL / STORE_LOCAL. Runtime-only; the
        // generic handlers above rewrite to these after observing a monomorphic
        // ValueType on the first dispatch. Each variant guards on its expected
        // tag and demotes to generic on miss (sticky).
        case OpCode::LOAD_LOCAL_INT: variableExecutor->handleLoadLocalInt(instr);
            break;
        case OpCode::LOAD_LOCAL_FLOAT: variableExecutor->handleLoadLocalFloat(instr);
            break;
        case OpCode::LOAD_LOCAL_BOOL: variableExecutor->handleLoadLocalBool(instr);
            break;
        case OpCode::LOAD_LOCAL_BOXED_INST: variableExecutor->handleLoadLocalBoxedInst(instr);
            break;
        case OpCode::STORE_LOCAL_INT: variableExecutor->handleStoreLocalInt(instr);
            break;
        case OpCode::STORE_LOCAL_FLOAT: variableExecutor->handleStoreLocalFloat(instr);
            break;
        case OpCode::STORE_LOCAL_BOOL: variableExecutor->handleStoreLocalBool(instr);
            break;
        case OpCode::STORE_LOCAL_BOXED_INST: variableExecutor->handleStoreLocalBoxedInst(instr);
            break;

        // MYT-202: compile-time superinstruction fusion. Handlers call the
        // slot-based fast-path entries (loadLocalSlot / storeLocalSlot) to
        // skip the per-dispatch Instruction::operands shim allocation that the
        // original implementation paid per fused op. Semantics are bit-
        // identical to the unfused LOAD_LOCAL + arith + STORE_LOCAL path.
        case OpCode::LOAD_LOAD_ADD_INT:
            variableExecutor->loadLocalSlot(instr.inlineOperands[0]);
            variableExecutor->loadLocalSlot(instr.inlineOperands[1]);
            arithmeticExecutor->handleAddInt();
            break;
        case OpCode::LOAD_LOAD_SUB_INT:
            variableExecutor->loadLocalSlot(instr.inlineOperands[0]);
            variableExecutor->loadLocalSlot(instr.inlineOperands[1]);
            arithmeticExecutor->handleSubInt();
            break;
        case OpCode::LOAD_LOAD_MUL_INT:
            variableExecutor->loadLocalSlot(instr.inlineOperands[0]);
            variableExecutor->loadLocalSlot(instr.inlineOperands[1]);
            arithmeticExecutor->handleMulInt();
            break;
        case OpCode::LOAD_GET_FIELD:
        {
            variableExecutor->loadLocalSlot(instr.inlineOperands[0]);
            // GET_FIELD still needs an Instruction shim — ObjectExecutor /
            // InlineCacheExecutor interfaces key off operands. Field fusion
            // fires far less often per iteration than LOAD_LOCAL pairs, so
            // the residual shim cost here is acceptable.
            bytecode::BytecodeProgram::Instruction getField(OpCode::GET_FIELD, instr.inlineOperands[1]);
            if (icEnabled && inlineCacheExecutor)
                inlineCacheExecutor->handleGetFieldIC(getField);
            else
                objectExecutor->handleGetField(getField);
            break;
        }
        case OpCode::LOAD_STORE_LOCAL:
            variableExecutor->loadLocalSlot(instr.inlineOperands[0]);
            variableExecutor->storeLocalSlot(instr.inlineOperands[1]);
            break;
        case OpCode::ADD_INT_STORE_LOCAL:
            arithmeticExecutor->handleAddInt();
            variableExecutor->storeLocalSlot(instr.inlineOperands[0]);
            break;
        case OpCode::OBJECT_TO_VALUE_CREATE_PROMISE:
        {
            // Fast path: if NEW_VALUE_OBJECT produced an Int box, skip both
            // the ValueObject conversion and the AsyncPromiseValue allocation
            // by extracting the inline int and pushing PROMISE_INT directly.
            // INVOKE_INT_GET_VALUE on the consuming side already handles raw
            // INT, so downstream semantics are preserved.
            const value::Value& tos = stackManager->peek(0);
            if (value::isObject(tos))
            {
                const auto& instance = value::asObject(tos);
                if (instance->getPrimitiveTag() == value::PrimitiveTypeTag::INT)
                {
                    value::Value field = instance->getFieldValue("value");
                    if (value::isInt(field))
                    {
                        int64_t raw = value::asInt(field);
                        stackManager->pop();
                        stackManager->push(value::Value(raw, value::Value::PromiseIntTag{}));
                        break;
                    }
                }
            }
            objectExecutor->handleObjectToValue(instr);
            value::Value val = stackManager->pop();
            auto promise = std::make_shared<value::AsyncPromiseValue>(val);
            stackManager->push(std::shared_ptr<value::PromiseValue>(promise));
            break;
        }
        case OpCode::CREATE_PROMISE_RETURN_VALUE:
        {
            // Inlined CREATE_PROMISE + RETURN_VALUE. Fast path for raw INT
            // returns: produce inline PROMISE_INT instead of allocating an
            // AsyncPromiseValue.
            value::Value val = stackManager->pop();
            if (value::isInt(val))
            {
                stackManager->push(value::Value(value::asInt(val), value::Value::PromiseIntTag{}));
            }
            else
            {
                auto promise = std::make_shared<value::AsyncPromiseValue>(val);
                stackManager->push(std::shared_ptr<value::PromiseValue>(promise));
            }
            controlFlowExecutor->handleReturnValue();
            break;
        }

        // Control flow
        case OpCode::JUMP: controlFlowExecutor->handleJump(instr);
            break;
        case OpCode::JUMP_IF_FALSE: controlFlowExecutor->handleJumpIfFalse(instr);
            break;
        case OpCode::JUMP_IF_TRUE: controlFlowExecutor->handleJumpIfTrue(instr);
            break;
        case OpCode::JUMP_IF_FALSE_OR_POP: controlFlowExecutor->handleJumpIfFalseOrPop(instr);
            break;
        case OpCode::JUMP_IF_TRUE_OR_POP: controlFlowExecutor->handleJumpIfTrueOrPop(instr);
            break;
        case OpCode::JUMP_BACK: controlFlowExecutor->handleJumpBack(instr);
            break;
        case OpCode::JUMP_IF_NULL: controlFlowExecutor->handleJumpIfNull(instr);
            break;
        case OpCode::SWITCH_STRING:
        {
            value::Value discriminant = stackManager->pop();
            size_t target = static_cast<size_t>(instr.inlineOperands[0]);
            const value::Value* stringValue = nullptr;
            value::Value boxedStringField;
            if (value::isAnyString(discriminant))
            {
                stringValue = &discriminant;
            }
            else if (value::isObject(discriminant))
            {
                const auto& instance = value::asObject(discriminant);
                if (instance && instance->getPrimitiveTag() == value::PrimitiveTypeTag::STRING)
                {
                    boxedStringField = instance->getFieldValue("value");
                    if (value::isAnyString(boxedStringField))
                    {
                        stringValue = &boxedStringField;
                    }
                }
            }
            else if (value::isValueObject(discriminant))
            {
                const auto& instance = value::asValueObject(discriminant);
                if (instance && instance->getPrimitiveTag() == value::PrimitiveTypeTag::STRING)
                {
                    boxedStringField = instance->getFieldValue("value");
                    if (value::isAnyString(boxedStringField))
                    {
                        stringValue = &boxedStringField;
                    }
                }
            }

            if (stringValue)
            {
                const std::string_view key = value::asStringView(*stringValue);
                const size_t caseCount = static_cast<size_t>(instr.inlineOperands[1]);
                const auto& constPool = activeProgram->getConstantPool();
                for (size_t c = 0; c < caseCount; ++c)
                {
                    const size_t stringOperand = 2 + c * 2;
                    const size_t targetOperand = stringOperand + 1;
                    const std::string& caseValue =
                        constPool.getString(static_cast<size_t>(instr.operandAt(stringOperand)));
                    if (key == caseValue)
                    {
                        target = static_cast<size_t>(instr.operandAt(targetOperand));
                        break;
                    }
                }
            }
            instructionPointer = target - 1;
            break;
        }
        case OpCode::RETURN: controlFlowExecutor->handleReturn();
            break;
        case OpCode::RETURN_VALUE: controlFlowExecutor->handleReturnValue();
            break;

        // Functions
        case OpCode::CALL:
            executeCallWithJit(instr);
            break;
        case OpCode::CALL_FAST:
            executeCallFastWithJit(instr);
            break;
        case OpCode::CALL_STATIC: functionExecutor->handleCallStatic(instr);
            break;

        // No-ops + halt
        case OpCode::HALT:
            // Handled in interpretLoop.
            break;
        case OpCode::NOP:
            break;
        case OpCode::LOOP_START:
        case OpCode::LOOP_END:
            // Markers for the loop optimizer.
            break;

        default:
            dispatchExtendedOps(instr);
            break;
        }
    }
}
