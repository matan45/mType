#include "ControlFlowExecutor.hpp"
#include "../../../value/PromiseValue.hpp"
#include "../../../value/AsyncPromiseValue.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include <iostream>
namespace vm::runtime
{
    ControlFlowExecutor::ControlFlowExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    void ControlFlowExecutor::handleJump(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("JUMP requires operand");
        }
        context.instructionPointer = instr.operands[0] - 1;  // -1 because loop increments
    }

    void ControlFlowExecutor::handleJumpIfFalse(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("JUMP_IF_FALSE requires operand");
        }
        value::Value condition = context.stackManager->pop();
        if (!isTruthy(condition)) {
            context.instructionPointer = instr.operands[0] - 1;
        }
    }

    void ControlFlowExecutor::handleJumpIfTrue(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("JUMP_IF_TRUE requires operand");
        }
        value::Value condition = context.stackManager->pop();
        if (isTruthy(condition)) {
            context.instructionPointer = instr.operands[0] - 1;
        }
    }

    void ControlFlowExecutor::handleJumpBack(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("JUMP_BACK requires operand");
        }
        // Jump back to loop start (operand is the target instruction)
        context.instructionPointer = instr.operands[0] - 1;  // -1 because loop increments
    }

    void ControlFlowExecutor::handleReturn() {
        if (context.callStack.empty()) {
            // Top-level return - halt execution
            context.instructionPointer = context.program->getInstructionCount();
        } else {
            CallFrame frame = context.callStack.back();

            // Notify debugger of function exit BEFORE popping the call stack
            if (debugger::DebugHookHelper::isDebuggingEnabled()) {
                debugger::DebugHookHelper::exitFunctionHook(frame.functionName);
            }

            context.callStack.pop_back();
            context.instructionPointer = frame.returnAddress;
            // Exit function scope (to clean up parameters and local variables)
            context.environment->exitScope();
            // Restore stack
            while (context.stackManager->size() > frame.frameBase) {
                context.stackManager->getStack().pop_back();
            }
        }
    }

    void ControlFlowExecutor::handleReturnValue() {
        value::Value returnVal = context.stackManager->pop();

        // Check if we're returning from an async function
        // If so, wrap the return value in a PromiseValue (unless already wrapped by CREATE_PROMISE)
        if (!context.callStack.empty()) {
            const CallFrame& frame = context.callStack.back();
            auto funcMetadata = context.program->getFunction(frame.functionName);

            if (funcMetadata && funcMetadata->isAsync) {
                // Check if already wrapped in Promise (by CREATE_PROMISE opcode)
                // This prevents double-wrapping when bytecode compiler emits CREATE_PROMISE
                if (!std::holds_alternative<std::shared_ptr<value::PromiseValue>>(returnVal)) {
                    // Wrap return value in AsyncPromiseValue for async functions
                    // Use AsyncPromiseValue to support event loop and callbacks
                    auto promise = std::make_shared<value::AsyncPromiseValue>(returnVal);
                    returnVal = promise;
                }
            }
        }

        handleReturn();
        context.stackManager->push(returnVal);
    }

    bool ControlFlowExecutor::isTruthy(const value::Value& val) const {
        if (std::holds_alternative<bool>(val)) {
            return std::get<bool>(val);
        }
        if (std::holds_alternative<int>(val)) {
            return std::get<int>(val) != 0;
        }
        if (std::holds_alternative<nullptr_t>(val)) {
            return false;
        }
        if (std::holds_alternative<std::monostate>(val)) {
            return false;
        }
        return true;  // Objects, strings, etc. are truthy
    }
}
