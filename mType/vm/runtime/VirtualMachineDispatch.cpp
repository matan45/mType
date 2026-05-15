#include "VirtualMachine.hpp"
#include <cstddef>
#include <cstdint>
#include <cassert>
#include "executors/StackOperationsExecutor.hpp"
#include "executors/ComparisonExecutor.hpp"
#include "executors/LogicalExecutor.hpp"
#include "executors/ArithmeticExecutor.hpp"
#include "executors/BitwiseExecutor.hpp"
#include "executors/ControlFlowExecutor.hpp"
#include "executors/VariableExecutor.hpp"
#include "executors/FunctionExecutor.hpp"
#include "executors/TypeExecutor.hpp"
#include "executors/ArrayExecutor.hpp"
#include "executors/ObjectExecutor.hpp"
#include "executors/LambdaExecutor.hpp"
#include "executors/ExceptionExecutor.hpp"
#include "executors/PrimitiveMethodExecutor.hpp"
#include "executors/InlineCacheExecutor.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/UserException.hpp"
#include "../../value/AsyncPromiseValue.hpp"
#include "../../value/ValueShim.hpp"
#include "../../value/PrimitiveTypeTag.hpp"
#include "../../debugger/DebugContext.hpp"
#include "../../debugger/DebugHookHelper.hpp"
#include "../jit/JitProfiler.hpp"
#include "../jit/JitCodeCache.hpp"
#include "../jit/JitCompiler.hpp"

namespace vm::runtime
{
    void VirtualMachine::executeInstruction(const bytecode::BytecodeProgram::Instruction& instr)
    {
        using OpCode = bytecode::OpCode;
        const auto* activeProgram = executionCtx ? executionCtx->program : program;

        switch (instr.opcode)
        {
        // Stack operations - delegated to StackOperationsExecutor
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

        // Arithmetic - delegated to ArithmeticExecutor
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

        // Comparison - delegated to ComparisonExecutor
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

        // Logical - delegated to LogicalExecutor
        case OpCode::AND: logicalExecutor->handleAnd();
            break;
        case OpCode::OR: logicalExecutor->handleOr();
            break;
        case OpCode::NOT: logicalExecutor->handleNot();
            break;

        // Bitwise - delegated to BitwiseExecutor
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

        // Variables - delegated to VariableExecutor
        case OpCode::LOAD_VAR: variableExecutor->handleLoadVar(instr);
            break;
        case OpCode::STORE_VAR: variableExecutor->handleStoreVar(instr);
            break;
        case OpCode::LOAD_VAR_CACHED:
        {
            // MYT-204: promoted from LOAD_VAR after the global-resolution
            // path stabilised. Side-table entry must exist (set by
            // tryPromoteLoadVarCached). Fall back to the generic handler if
            // somehow reached without an entry (e.g., side table cleared).
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
        // MYT-199: type-quickened LOAD_LOCAL / STORE_LOCAL. Runtime-only;
        // the generic handlers above rewrite to these after observing a
        // monomorphic ValueType on the first dispatch. Each variant guards
        // on its expected tag and demotes to generic on miss (sticky).
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
        // skip the per-dispatch Instruction::operands shim allocation that
        // the original implementation paid per fused op. Semantics are bit-
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
            // Fallback: original two-allocation path.
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

        // Control flow - delegated to ControlFlowExecutor
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
        case OpCode::RETURN: controlFlowExecutor->handleReturn();
            break;
        case OpCode::RETURN_VALUE: controlFlowExecutor->handleReturnValue();
            break;

        // Functions - with JIT dispatch
        case OpCode::CALL:
            executeCallWithJit(instr);
            break;
        case OpCode::CALL_FAST:
            executeCallFastWithJit(instr);
            break;
        case OpCode::CALL_STATIC: functionExecutor->handleCallStatic(instr);
            break;

        // Objects - delegated to ObjectExecutor
        case OpCode::NEW_OBJECT: objectExecutor->handleNewObject(instr);
            break;
        case OpCode::NEW_STACK: objectExecutor->handleNewStack(instr);
            break;
        case OpCode::NEW_VALUE_OBJECT: objectExecutor->handleNewValueObject(instr);
            break;
        case OpCode::OBJECT_TO_VALUE: objectExecutor->handleObjectToValue(instr);
            break;
        case OpCode::GET_FIELD:
            if (icEnabled && inlineCacheExecutor)
                inlineCacheExecutor->handleGetFieldIC(instr);
            else
                objectExecutor->handleGetField(instr);
            break;
        case OpCode::GET_FIELD_TYPED:
            // MYT-212: class-targeted field read. IC-bypass — the static type
            // hint is the optimisation; no shape cache layered on top.
            objectExecutor->handleGetFieldTyped(instr);
            break;
        case OpCode::SET_FIELD_TYPED:
            objectExecutor->handleSetFieldTyped(instr);
            break;
        // MYT-274 Phase 2: structural-equality fused opcodes for synthesized
        // hashCode/equals on int-only classes — see OpCode.hpp.
        case OpCode::STRUCT_HASH_INT:
            objectExecutor->handleStructHashInt(instr);
            break;
        case OpCode::STRUCT_EQ_INT:
            objectExecutor->handleStructEqInt(instr);
            break;
        case OpCode::GET_FIELD_CACHED:
        {
            // MYT-194: promoted from GET_FIELD once the IC stabilized. Never
            // emitted by the compiler; only reachable if IC is enabled (the
            // promoter guards on that). Fall back to the generic handler if
            // somehow reached with IC disabled.
            if (icEnabled && inlineCacheExecutor)
            {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                if (!state)
                    throw errors::RuntimeException("GET_FIELD_CACHED dispatched without side-table entry");
                inlineCacheExecutor->handleGetFieldCached(instr, *state);
            }
            else
                objectExecutor->handleGetField(instr);
            break;
        }
        case OpCode::LOAD_LOCAL_GET_FIELD_CACHED:
        {
            // MYT-198: fused LOAD_LOCAL + GET_FIELD_CACHED. IC-only; if IC is
            // disabled fall back by materialising the LOAD_LOCAL and running
            // the generic GET_FIELD. state.fusedSlot carries the receiver slot.
            if (icEnabled && inlineCacheExecutor)
            {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                if (!state)
                    throw errors::RuntimeException("LOAD_LOCAL_GET_FIELD_CACHED dispatched without side-table entry");
                inlineCacheExecutor->handleLoadLocalGetFieldCached(instr, *state);
            }
            else {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                uint64_t slot = state ? static_cast<uint64_t>(state->fusedSlot) : 0;
                variableExecutor->handleLoadLocal(
                    bytecode::BytecodeProgram::Instruction(
                        bytecode::OpCode::LOAD_LOCAL, slot));
                objectExecutor->handleGetField(instr);
            }
            break;
        }
        case OpCode::SET_FIELD:
            if (icEnabled && inlineCacheExecutor)
                inlineCacheExecutor->handleSetFieldIC(instr);
            else
                objectExecutor->handleSetField(instr);
            break;
        case OpCode::SET_FIELD_CACHED:
        {
            // MYT-194: see GET_FIELD_CACHED above.
            if (icEnabled && inlineCacheExecutor)
            {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                if (!state)
                    throw errors::RuntimeException("SET_FIELD_CACHED dispatched without side-table entry");
                inlineCacheExecutor->handleSetFieldCached(instr, *state);
            }
            else
                objectExecutor->handleSetField(instr);
            break;
        }
        case OpCode::INLINE_SET_FIELD:
            if (icEnabled && inlineCacheExecutor)
                inlineCacheExecutor->handleInlineSetFieldIC(instr);
            else
                objectExecutor->handleInlineSetField(instr);
            break;
        case OpCode::INLINE_GET_FIELD:
            if (icEnabled && inlineCacheExecutor)
                inlineCacheExecutor->handleInlineGetFieldIC(instr);
            else
                objectExecutor->handleInlineGetField(instr);
            break;
        case OpCode::GET_STATIC: objectExecutor->handleGetStatic(instr);
            break;
        case OpCode::SET_STATIC: objectExecutor->handleSetStatic(instr);
            break;
        case OpCode::CALL_METHOD:
        case OpCode::INVOKE:
            if (icEnabled && inlineCacheExecutor)
                inlineCacheExecutor->handleCallMethodIC(instr);
            else
                objectExecutor->handleCallMethod(instr);
            break;
        case OpCode::CALL_METHOD_CACHED:
        {
            // MYT-173: promoted from CALL_METHOD once the IC stabilized. Never
            // emitted by the compiler; only reachable if IC is enabled (the
            // promoter guards on that). Fall back to the generic handler if
            // somehow reached with IC disabled.
            if (icEnabled && inlineCacheExecutor)
            {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                if (!state)
                    throw errors::RuntimeException("CALL_METHOD_CACHED dispatched without side-table entry");
                inlineCacheExecutor->handleCallMethodCached(instr, *state);
            }
            else
                objectExecutor->handleCallMethod(instr);
            break;
        }
        case OpCode::CALL_METHOD_POLY_CACHED:
        {
            // MYT-203: promoted from CALL_METHOD once the IC reaches POLY
            // (2-4 entries). Same RUNTIME-ONLY + IC-only constraints as
            // CALL_METHOD_CACHED.
            if (icEnabled && inlineCacheExecutor)
            {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                if (!state)
                    throw errors::RuntimeException("CALL_METHOD_POLY_CACHED dispatched without side-table entry");
                inlineCacheExecutor->handleCallMethodPolyCached(instr, *state);
            }
            else
                objectExecutor->handleCallMethod(instr);
            break;
        }
        case OpCode::LOAD_LOCAL_CALL_CACHED:
        {
            // MYT-198: fused LOAD_LOCAL + CALL_METHOD_CACHED. Same IC-only
            // constraint as CALL_METHOD_CACHED. state.fusedSlot carries the
            // receiver slot that the NOPed LOAD_LOCAL would have pushed.
            if (icEnabled && inlineCacheExecutor)
            {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                if (!state)
                    throw errors::RuntimeException("LOAD_LOCAL_CALL_CACHED dispatched without side-table entry");
                inlineCacheExecutor->handleLoadLocalCallCached(instr, *state);
            }
            else {
                const auto* state = activeProgram->findCachedState(instructionPointer);
                uint64_t slot = state ? static_cast<uint64_t>(state->fusedSlot) : 0;
                variableExecutor->handleLoadLocal(
                    bytecode::BytecodeProgram::Instruction(
                        bytecode::OpCode::LOAD_LOCAL, slot));
                objectExecutor->handleCallMethod(instr);
            }
            break;
        }
        case OpCode::LOAD_LOCAL_CALL_POLY_CACHED:
        {
            // MYT-203: fused LOAD_LOCAL + CALL_METHOD_POLY_CACHED. Symmetric
            // to LOAD_LOCAL_CALL_CACHED above.
            const auto* state = activeProgram->findCachedState(instructionPointer);
            if (icEnabled && inlineCacheExecutor)
            {
                if (!state)
                    throw errors::RuntimeException("LOAD_LOCAL_CALL_POLY_CACHED dispatched without side-table entry");
                inlineCacheExecutor->handleLoadLocalCallPolyCached(instr, *state);
            }
            else {
                // IC disabled: replay the fused pair via the generic handlers.
                // Falls back to slot 0 if the side-table entry vanished — same
                // tolerance the IC-enabled path uses for missing state plus an
                // explicit error (here we keep going with a defaulted slot
                // since the generic CALL_METHOD will fail with the right error
                // if the receiver isn't where it should be).
                uint64_t slot = state ? static_cast<uint64_t>(state->fusedSlot) : 0;
                variableExecutor->handleLoadLocal(
                    bytecode::BytecodeProgram::Instruction(
                        bytecode::OpCode::LOAD_LOCAL, slot));
                objectExecutor->handleCallMethod(instr);
            }
            break;
        }
        case OpCode::SUPER_CONSTRUCTOR: objectExecutor->handleSuperConstructor(instr);
            break;
        case OpCode::THIS_CONSTRUCTOR: objectExecutor->handleThisConstructor(instr);
            break;
        case OpCode::SUPER_INVOKE: objectExecutor->handleSuperInvoke(instr);
            break;
        case OpCode::SUPER_GET_FIELD: objectExecutor->handleSuperGetField(instr);
            break;
        case OpCode::SUPER_SET_FIELD: objectExecutor->handleSuperSetField(instr);
            break;

        // Iterator Operations
        case OpCode::GET_ITERATOR: objectExecutor->handleGetIterator(instr);
            break;
        case OpCode::ITERATOR_HAS_NEXT: objectExecutor->handleIteratorHasNext(instr);
            break;
        case OpCode::ITERATOR_NEXT: objectExecutor->handleIteratorNext(instr);
            break;
        case OpCode::ITERATOR_CLOSE: objectExecutor->handleIteratorClose(instr);
            break;

        // Primitive Method Optimizations - delegated to PrimitiveMethodExecutor (Phase 3)
        // Int object methods
        case OpCode::INVOKE_INT_ADD: primitiveMethodExecutor->handleInvokeIntAdd();
            break;
        case OpCode::INVOKE_INT_SUB: primitiveMethodExecutor->handleInvokeIntSub();
            break;
        case OpCode::INVOKE_INT_MUL: primitiveMethodExecutor->handleInvokeIntMul();
            break;
        case OpCode::INVOKE_INT_DIV: primitiveMethodExecutor->handleInvokeIntDiv();
            break;
        case OpCode::INVOKE_INT_MOD: primitiveMethodExecutor->handleInvokeIntMod();
            break;
        case OpCode::INVOKE_INT_NEG: primitiveMethodExecutor->handleInvokeIntNeg();
            break;
        case OpCode::INVOKE_INT_ABS: primitiveMethodExecutor->handleInvokeIntAbs();
            break;
        case OpCode::INVOKE_INT_EQUALS: primitiveMethodExecutor->handleInvokeIntEquals();
            break;
        case OpCode::INVOKE_INT_COMPARE: primitiveMethodExecutor->handleInvokeIntCompare();
            break;
        case OpCode::INVOKE_INT_GET_VALUE: primitiveMethodExecutor->handleInvokeIntGetValue();
            break;
        case OpCode::INVOKE_INT_LESS_THAN: primitiveMethodExecutor->handleInvokeIntLessThan();
            break;
        case OpCode::INVOKE_INT_LESS_EQUAL: primitiveMethodExecutor->handleInvokeIntLessEqual();
            break;
        case OpCode::INVOKE_INT_GREATER_THAN: primitiveMethodExecutor->handleInvokeIntGreaterThan();
            break;
        case OpCode::INVOKE_INT_GREATER_EQUAL: primitiveMethodExecutor->handleInvokeIntGreaterEqual();
            break;

        // Float object methods
        case OpCode::INVOKE_FLOAT_ADD: primitiveMethodExecutor->handleInvokeFloatAdd();
            break;
        case OpCode::INVOKE_FLOAT_SUB: primitiveMethodExecutor->handleInvokeFloatSub();
            break;
        case OpCode::INVOKE_FLOAT_MUL: primitiveMethodExecutor->handleInvokeFloatMul();
            break;
        case OpCode::INVOKE_FLOAT_DIV: primitiveMethodExecutor->handleInvokeFloatDiv();
            break;
        case OpCode::INVOKE_FLOAT_NEG: primitiveMethodExecutor->handleInvokeFloatNeg();
            break;
        case OpCode::INVOKE_FLOAT_ABS: primitiveMethodExecutor->handleInvokeFloatAbs();
            break;
        case OpCode::INVOKE_FLOAT_EQUALS: primitiveMethodExecutor->handleInvokeFloatEquals();
            break;
        case OpCode::INVOKE_FLOAT_COMPARE: primitiveMethodExecutor->handleInvokeFloatCompare();
            break;
        case OpCode::INVOKE_FLOAT_GET_VALUE: primitiveMethodExecutor->handleInvokeFloatGetValue();
            break;
        case OpCode::INVOKE_BOOL_GET_VALUE: primitiveMethodExecutor->handleInvokeBoolGetValue();
            break;
        case OpCode::INVOKE_FLOAT_LESS_THAN: primitiveMethodExecutor->handleInvokeFloatLessThan();
            break;
        case OpCode::INVOKE_FLOAT_LESS_EQUAL: primitiveMethodExecutor->handleInvokeFloatLessEqual();
            break;
        case OpCode::INVOKE_FLOAT_GREATER_THAN: primitiveMethodExecutor->handleInvokeFloatGreaterThan();
            break;
        case OpCode::INVOKE_FLOAT_GREATER_EQUAL: primitiveMethodExecutor->handleInvokeFloatGreaterEqual();
            break;

        // Bool object methods
        case OpCode::INVOKE_BOOL_AND: primitiveMethodExecutor->handleInvokeBoolAnd();
            break;
        case OpCode::INVOKE_BOOL_OR: primitiveMethodExecutor->handleInvokeBoolOr();
            break;
        case OpCode::INVOKE_BOOL_XOR: primitiveMethodExecutor->handleInvokeBoolXor();
            break;
        case OpCode::INVOKE_BOOL_NOT: primitiveMethodExecutor->handleInvokeBoolNot();
            break;
        case OpCode::INVOKE_BOOL_EQUALS: primitiveMethodExecutor->handleInvokeBoolEquals();
            break;

        // String object methods
        case OpCode::INVOKE_STRING_LENGTH: primitiveMethodExecutor->handleInvokeStringLength();
            break;
        case OpCode::INVOKE_STRING_CONCAT: primitiveMethodExecutor->handleInvokeStringConcat();
            break;
        case OpCode::INVOKE_STRING_EQUALS: primitiveMethodExecutor->handleInvokeStringEquals();
            break;
        case OpCode::INVOKE_STRING_IS_EMPTY: primitiveMethodExecutor->handleInvokeStringIsEmpty();
            break;

        // Arrays - delegated to ArrayExecutor
        case OpCode::NEW_ARRAY: arrayExecutor->handleNewArray(instr);
            break;
        case OpCode::NEW_ARRAY_MULTI: arrayExecutor->handleNewArrayMulti(instr);
            break;
        case OpCode::ARRAY_GET: arrayExecutor->handleArrayGet();
            break;
        case OpCode::ARRAY_GET_ALIAS: arrayExecutor->handleArrayGetAlias();
            break;
        case OpCode::ARRAY_SET: arrayExecutor->handleArraySet();
            break;
        case OpCode::ARRAY_LENGTH: arrayExecutor->handleArrayLength();
            break;

        // SoA Field Access Optimization
        case OpCode::ARRAY_GET_FIELD: arrayExecutor->handleArrayGetField(instr);
            break;
        case OpCode::ARRAY_SET_FIELD: arrayExecutor->handleArraySetField(instr);
            break;
        case OpCode::ARRAY_GET_INT_LOCAL: arrayExecutor->handleArrayGetIntLocal(instr);
            break;
        case OpCode::ARRAY_SET_INT_LOCAL: arrayExecutor->handleArraySetIntLocal(instr);
            break;
        case OpCode::ARRAY_LENGTH_LOCAL: arrayExecutor->handleArrayLengthLocal(instr);
            break;

        // Type operations - delegated to TypeExecutor
        case OpCode::INSTANCEOF: typeExecutor->handleInstanceof(instr);
            break;
        case OpCode::INSTANCEOF_TYPEPARAM: typeExecutor->handleInstanceofTypeParam(instr);
            break;
        case OpCode::CAST: typeExecutor->handleCast(instr);
            break;
        case OpCode::CAST_TYPEPARAM: typeExecutor->handleCastTypeParam(instr);
            break;
        case OpCode::BIND_TYPE_ARGS: typeExecutor->handleBindTypeArgs(instr);
            break;

        // Lambda operations - delegated to LambdaExecutor
        case OpCode::LAMBDA: lambdaExecutor->handleLambda(instr);
            break;

        // Exception handling - delegated to ExceptionExecutor
        case OpCode::TRY_BEGIN:
            // Entering a new try block - check if we have a pending exception to re-throw
            if (pendingException != nullptr)
            {
                errors::UserException exToRethrow = *pendingException;
                pendingException.reset();
                pendingFinallyOffset = SIZE_MAX;
                currentFinallyOffset = SIZE_MAX;
                throw exToRethrow;
            }
            // Entering a new try block - we're no longer in a finally (if we were)
            currentFinallyOffset = SIZE_MAX;
            exceptionExecutor->handleTryBegin(instr);
            break;
        case OpCode::TRY_END:
            // TRY_END marks end of try body OR end of entire try-catch-finally construct
            // If we have a pending exception, this means we've exited the finally block, so re-throw
            if (pendingException != nullptr)
            {
                errors::UserException exToRethrow = *pendingException;
                pendingException.reset();
                pendingFinallyOffset = SIZE_MAX;
                currentFinallyOffset = SIZE_MAX;
                throw exToRethrow;
            }
            // Reset currentFinallyOffset - we've exited the finally block
            currentFinallyOffset = SIZE_MAX;
            exceptionExecutor->handleTryEnd(instr);
            break;
        case OpCode::CATCH: exceptionExecutor->handleCatch(instr);
            break;
        case OpCode::THROW: exceptionExecutor->handleThrow(instr);
            break;
        case OpCode::FINALLY:
            // Mark that we're entering this finally block
            currentFinallyOffset = instructionPointer;
            exceptionExecutor->handleFinally(instr);
            break;

        // Special
        case OpCode::HALT:
            // Handled in interpretLoop
            break;

        case OpCode::NOP:
            // Do nothing
            break;

        // Loop optimization markers (no-op at runtime, used by optimizer)
        case OpCode::LOOP_START:
        case OpCode::LOOP_END:
            // No-op: these are markers for the loop optimizer
            break;

        // Async/Await Operations
        case OpCode::CREATE_PROMISE:
            {
                // Pop value from stack and wrap it in a Promise. Fast path:
                // raw INT inputs collapse to the inline PROMISE_INT form (no
                // heap allocation). All other inputs allocate AsyncPromiseValue
                // for full event-loop / callback support.
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
                break;
            }

        case OpCode::AWAIT:
            executeAwait();
            break;

        case OpCode::PROMISE_RESOLVE:
            // Reserved for future use — no emitter generates this opcode
            break;

        // Debug opcodes
        case OpCode::BREAKPOINT:
            // Breakpoint hit - pause execution
            if (debuggingEnabled && debugger::DebugHookHelper::isDebuggingEnabled())
            {
                auto sourceLoc = program->getSourceLocation(instructionPointer);
                if (sourceLoc)
                {
                    // Convert BytecodeProgram::SourceLocation to errors::SourceLocation
                    errors::SourceLocation location(sourceLoc->filename,
                                                    static_cast<int>(sourceLoc->line),
                                                    static_cast<int>(sourceLoc->column));

                    // Always pause at explicit breakpoint
                    if (debugger::DebugHookHelper::shouldPause(location))
                    {
                        debugger::DebugHookHelper::waitForResume();
                    }
                }
            }
            break;

        case OpCode::LINE:
            // Update current source line
            if (instr.hasOperands())
            {
                currentSourceLine = static_cast<int>(instr.inlineOperands[0]);
            }
            break;

        case OpCode::SOURCE_FILE:
            // Update current source file from constant pool
            if (instr.hasOperands())
            {
                size_t stringIndex = instr.inlineOperands[0];
                const auto& constPool = program->getConstantPool();
                if (stringIndex < constPool.strings.size())
                {
                    currentSourceFile = constPool.getString(stringIndex);
                }
            }
            break;

        case OpCode::PROFILE_ENTER:
            if (jitEnabled && jitProfiler && !callStack.empty())
            {
                // MYT-197: resolve the frame's handle to the owning program's
                // interned name. JitProfiler + JitCompiler still key on
                // std::string (cold path — PROFILE_ENTER only fires while
                // tiering up).
                const auto& frame = callStack.back();
                const bytecode::BytecodeProgram* framePrg =
                    (frame.programIndex < loadedPrograms.size())
                        ? loadedPrograms[frame.programIndex]
                        : program;
                const std::string& funcName = framePrg->getFrameName(frame.functionName);
                bool justBecameHot = jitProfiler->recordEntry(funcName);
                if (justBecameHot && jitCompiler && jitCodeCache)
                {
                    // MYT-314: compile against framePrg (the program owning
                    // this frame), not the top-level `program`. JitCompiler::
                    // compile does program.getFunction(funcName), which would
                    // silently bail out for imported functions if we passed
                    // the top-level program.
                    jitCompiler->compile(funcName, *framePrg, *jitCodeCache, typeFeedbackCollector.get());
                }
            }
            break;
        case OpCode::PROFILE_EXIT:
            // No-op for now — profiling uses entry counts only
            break;

        default:
            throw errors::RuntimeException("Unimplemented opcode: " +
                std::string(bytecode::getOpCodeName(instr.opcode)));
        }
    }
}
