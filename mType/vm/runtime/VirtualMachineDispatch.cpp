#include "VirtualMachine.hpp"
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
            trySpecializeArithmetic(instr, OpCode::ADD_INT);
            arithmeticExecutor->handleAdd();
            break;
        case OpCode::STRING_BUILD:
            arithmeticExecutor->handleStringBuild(static_cast<size_t>(instr.operands[0]));
            break;
        case OpCode::SUB:
            trySpecializeArithmetic(instr, OpCode::SUB_INT);
            arithmeticExecutor->handleSub();
            break;
        case OpCode::MUL:
            trySpecializeArithmetic(instr, OpCode::MUL_INT);
            arithmeticExecutor->handleMul();
            break;
        case OpCode::DIV:
            trySpecializeArithmetic(instr, OpCode::DIV_INT);
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
        case OpCode::SUB_INT: arithmeticExecutor->handleSubInt();
            break;
        case OpCode::MUL_INT: arithmeticExecutor->handleMulInt();
            break;
        case OpCode::DIV_INT: arithmeticExecutor->handleDivInt();
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
        case OpCode::BITWISE_AND_OP: bitwiseExecutor->handleBitwiseAnd();
            break;
        case OpCode::BITWISE_OR_OP: bitwiseExecutor->handleBitwiseOr();
            break;
        case OpCode::BITWISE_XOR_OP: bitwiseExecutor->handleBitwiseXor();
            break;
        case OpCode::LEFT_SHIFT_OP: bitwiseExecutor->handleLeftShift();
            break;
        case OpCode::RIGHT_SHIFT_OP: bitwiseExecutor->handleRightShift();
            break;
        case OpCode::BITWISE_NOT_OP: bitwiseExecutor->handleBitwiseNot();
            break;

        // Variables - delegated to VariableExecutor
        case OpCode::LOAD_VAR: variableExecutor->handleLoadVar(instr);
            break;
        case OpCode::STORE_VAR: variableExecutor->handleStoreVar(instr);
            break;
        case OpCode::DECLARE_VAR: variableExecutor->handleDeclareVar(instr);
            break;
        case OpCode::LOAD_LOCAL: variableExecutor->handleLoadLocal(instr);
            break;
        case OpCode::STORE_LOCAL: variableExecutor->handleStoreLocal(instr);
            break;

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
        case OpCode::CALL_STATIC: functionExecutor->handleCallStatic(instr);
            break;

        // Objects - delegated to ObjectExecutor
        case OpCode::NEW_OBJECT: objectExecutor->handleNewObject(instr);
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
        case OpCode::SET_FIELD:
            if (icEnabled && inlineCacheExecutor)
                inlineCacheExecutor->handleSetFieldIC(instr);
            else
                objectExecutor->handleSetField(instr);
            break;
        case OpCode::INLINE_SET_FIELD:
            if (icEnabled && inlineCacheExecutor)
                inlineCacheExecutor->handleInlineSetFieldIC(instr);
            else
                objectExecutor->handleInlineSetField(instr);
            break;
        case OpCode::GET_STATIC: objectExecutor->handleGetStatic(instr);
            break;
        case OpCode::SET_STATIC: objectExecutor->handleSetStatic(instr);
            break;
        case OpCode::CALL_METHOD:
            if (icEnabled && inlineCacheExecutor)
                inlineCacheExecutor->handleCallMethodIC(instr);
            else
                objectExecutor->handleCallMethod(instr);
            break;
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

        // Arrays - delegated to ArrayExecutor
        case OpCode::NEW_ARRAY: arrayExecutor->handleNewArray(instr);
            break;
        case OpCode::NEW_ARRAY_MULTI: arrayExecutor->handleNewArrayMulti(instr);
            break;
        case OpCode::ARRAY_GET: arrayExecutor->handleArrayGet();
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

        // Lambda operations - delegated to LambdaExecutor
        case OpCode::LAMBDA: lambdaExecutor->handleLambda(instr);
            break;
        case OpCode::LAMBDA_INVOKE: lambdaExecutor->handleLambdaInvoke(instr);
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
                // Pop value from stack and wrap it in a Promise
                // Use AsyncPromiseValue for async functions so they integrate with event loop
                value::Value val = stackManager->pop();
                auto promise = std::make_shared<value::AsyncPromiseValue>(val);
                stackManager->push(promise);
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
            if (!instr.operands.empty())
            {
                currentSourceLine = static_cast<int>(instr.operands[0]);
            }
            break;

        case OpCode::SOURCE_FILE:
            // Update current source file from constant pool
            if (!instr.operands.empty())
            {
                size_t stringIndex = instr.operands[0];
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
                const std::string& funcName = callStack.back().functionName;
                bool justBecameHot = jitProfiler->recordEntry(funcName);
                if (justBecameHot && jitCompiler && jitCodeCache)
                {
                    jitCompiler->compile(funcName, *program, *jitCodeCache, typeFeedbackCollector.get());
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
