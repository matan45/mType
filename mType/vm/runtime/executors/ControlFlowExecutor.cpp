#include "ControlFlowExecutor.hpp"
#include "../../../value/PromiseValue.hpp"
#include "../../../value/AsyncPromiseValue.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../profiler/ProfilerHookHelper.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../jit/OSRManager.hpp"
#include "../VirtualMachine.hpp"
#include "../utils/NullCheckUtils.hpp"
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

    void ControlFlowExecutor::handleJumpIfFalseOrPop(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("JUMP_IF_FALSE_OR_POP requires operand");
        }
        // Peek at the value without popping
        value::Value condition = context.stackManager->peek();
        if (!isTruthy(condition)) {
            // If false, jump (keeping the false value on stack as result)
            context.instructionPointer = instr.operands[0] - 1;
        } else {
            // If true, pop it and continue to evaluate the right side
            context.stackManager->pop();
        }
    }

    void ControlFlowExecutor::handleJumpIfTrueOrPop(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("JUMP_IF_TRUE_OR_POP requires operand");
        }
        // Peek at the value without popping
        value::Value condition = context.stackManager->peek();
        if (isTruthy(condition)) {
            // If true, jump (keeping the true value on stack as result)
            context.instructionPointer = instr.operands[0] - 1;
        } else {
            // If false, pop it and continue to evaluate the right side
            context.stackManager->pop();
        }
    }

    void ControlFlowExecutor::handleJumpBack(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("JUMP_BACK requires operand");
        }

        // Phase 5: OSR check at loop back-edge
        if (osrManager && context.vm && context.vm->isJitEnabled())
        {
            auto* compiler = context.vm->getJitCompiler();
            auto* codeCache = context.vm->getJitCodeCache();
            if (compiler && codeCache)
            {
                if (osrManager->tryOSR(context.instructionPointer,
                                        *context.program,
                                        context,
                                        *context.vm,
                                        *compiler,
                                        *codeCache))
                {
                    // OSR executed the loop natively and set instructionPointer
                    return;
                }
            }
        }

        // Normal interpreter path: jump back to loop start
        context.instructionPointer = instr.operands[0] - 1;  // -1 because loop increments
    }

    void ControlFlowExecutor::handleJumpIfNull(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("JUMP_IF_NULL requires operand");
        }
        value::Value val = context.stackManager->pop();
        if (utils::isNullValue(val)) {
            context.instructionPointer = instr.operands[0] - 1;
        }
    }

    void ControlFlowExecutor::handleReturn() {
        if (context.callStack.empty()) {
            // Top-level return - halt execution
            context.instructionPointer = context.program->getInstructionCount();
        } else {
            CallFrame frame = context.callStack.back();

            // Notify profiler of function exit BEFORE popping the call stack
            if (vm::profiler::ProfilerHookHelper::isProfilingEnabled()) {
                vm::profiler::ProfilerHookHelper::onFunctionExit(frame.functionName);
            }

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
        return utils::isTruthy(val);
    }
}
