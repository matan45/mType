#include "StackOperationsExecutor.hpp"
#include "../../../value/StringPool.hpp"

namespace vm::runtime
{
    StackOperationsExecutor::StackOperationsExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    void StackOperationsExecutor::handlePushInt(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("PUSH_INT requires operand");
        }
        int64_t value = context.program->getConstantPool().getInteger(instr.operands[0]);
        context.stackManager->push(value);
    }

    void StackOperationsExecutor::handlePushFloat(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("PUSH_FLOAT requires operand");
        }
        double value = context.program->getConstantPool().getFloat(instr.operands[0]);
        context.stackManager->push(static_cast<float>(value));
    }

    void StackOperationsExecutor::handlePushString(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("PUSH_STRING requires operand");
        }
        const std::string& value = context.program->getConstantPool().getString(instr.operands[0]);
        // Use StringPool to intern the string
        auto& pool = value::StringPool::getInstance();
        context.stackManager->push(pool.intern(value));
    }

    void StackOperationsExecutor::handlePushBool(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.empty()) {
            throw errors::RuntimeException("PUSH_BOOL requires operand");
        }
        context.stackManager->push(instr.operands[0] != 0);
    }

    void StackOperationsExecutor::handlePushNull() {
        context.stackManager->push(nullptr);
    }

    void StackOperationsExecutor::handlePop() {
        context.stackManager->pop();
    }

    void StackOperationsExecutor::handleDup() {
        context.stackManager->push(context.stackManager->peek());
    }

    void StackOperationsExecutor::handleSwap() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: SWAP requires 2 values");
        }
        value::Value top = context.stackManager->pop();
        value::Value second = context.stackManager->pop();
        context.stackManager->push(top);
        context.stackManager->push(second);
    }
}
