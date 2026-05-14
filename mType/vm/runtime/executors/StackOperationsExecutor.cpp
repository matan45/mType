#include "StackOperationsExecutor.hpp"
#include <cstdint>
#include "../../../value/StringPool.hpp"

namespace vm::runtime
{
    StackOperationsExecutor::StackOperationsExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    // MYT-318: PUSH_* operand-count contract is enforced by
    // BytecodeProgram::validateInstructionOperands at program load, so the
    // runtime defensive `if (numOperands() == 0) throw` checks are gone here.

    void StackOperationsExecutor::handlePushInt(const bytecode::BytecodeProgram::Instruction& instr) {
        int64_t value = context.program->getConstantPool().getInteger(instr.inlineOperands[0]);
        context.stackManager->push(value);
    }

    void StackOperationsExecutor::handlePushFloat(const bytecode::BytecodeProgram::Instruction& instr) {
        double value = context.program->getConstantPool().getFloat(instr.inlineOperands[0]);
        context.stackManager->push(value);
    }

    void StackOperationsExecutor::handlePushString(const bytecode::BytecodeProgram::Instruction& instr) {
        const std::string& value = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        auto& pool = value::StringPool::getInstance();
        context.stackManager->push(pool.intern(value));
    }

    void StackOperationsExecutor::handlePushBool(const bytecode::BytecodeProgram::Instruction& instr) {
        context.stackManager->push(instr.inlineOperands[0] != 0);
    }

    void StackOperationsExecutor::handlePushNull() {
        context.stackManager->push(nullptr);
    }

    void StackOperationsExecutor::handlePop() {
        context.stackManager->pop();
    }

    void StackOperationsExecutor::handleDup() {
        // MYT-318: copy the top slot without an intermediate peek-by-value.
        // The push may reallocate the underlying vector, which would
        // invalidate any reference into it — so we copy through a local first.
        if (context.stackManager->empty()) {
            throw errors::RuntimeException("Stack underflow: DUP requires 1 value");
        }
        value::Value copy = context.stackManager->peekRef(0);
        context.stackManager->push(std::move(copy));
    }

    void StackOperationsExecutor::handleSwap() {
        // MYT-318: swap in place via std::swap on references — no pop/push,
        // no size-field thrash, no temporaries beyond the swap itself.
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: SWAP requires 2 values");
        }
        auto& top    = context.stackManager->peekRef(0);
        auto& second = context.stackManager->peekRef(1);
        std::swap(top, second);
    }
}
