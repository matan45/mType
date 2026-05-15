#pragma once
#include <cstdint>
#include <utility>
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../value/StringPool.hpp"

namespace vm::runtime
{
    /**
     * Executes stack manipulation opcodes
     * Handles PUSH_*, POP, DUP, SWAP operations
     */
    class StackOperationsExecutor
    {
    public:
        explicit StackOperationsExecutor(ExecutionContext& ctx) : context(ctx) {}
        ~StackOperationsExecutor() = default;

        // MYT-319: handler bodies live in the header so MSVC v145 (no /GL)
        // can inline them through the unique_ptr<StackOperationsExecutor>
        // pointer dereference in VirtualMachineDispatch.cpp.
        //
        // MYT-318: PUSH_* operand-count contract is enforced by
        // BytecodeProgram::validateInstructionOperands at program load, so
        // runtime defensive operand-count checks are gone here.

        inline void handlePushInt(const bytecode::BytecodeProgram::Instruction& instr) {
            int64_t value = context.program->getConstantPool().getInteger(instr.inlineOperands[0]);
            context.stackManager->push(value);
        }

        inline void handlePushFloat(const bytecode::BytecodeProgram::Instruction& instr) {
            double value = context.program->getConstantPool().getFloat(instr.inlineOperands[0]);
            context.stackManager->push(value);
        }

        inline void handlePushString(const bytecode::BytecodeProgram::Instruction& instr) {
            const std::string& value = context.program->getConstantPool().getString(instr.inlineOperands[0]);
            auto& pool = value::StringPool::getInstance();
            context.stackManager->push(pool.intern(value));
        }

        inline void handlePushBool(const bytecode::BytecodeProgram::Instruction& instr) {
            context.stackManager->push(instr.inlineOperands[0] != 0);
        }

        inline void handlePushNull() {
            context.stackManager->push(nullptr);
        }

        inline void handlePop() {
            context.stackManager->pop();
        }

        inline void handleDup() {
            // MYT-318: copy the top slot without an intermediate peek-by-value.
            // The push may reallocate the underlying vector, which would
            // invalidate any reference into it — so we copy through a local first.
            if (context.stackManager->empty()) {
                throw errors::RuntimeException("Stack underflow: DUP requires 1 value");
            }
            value::Value copy = context.stackManager->peekRef(0);
            context.stackManager->push(std::move(copy));
        }

        inline void handleSwap() {
            // MYT-318: swap in place via std::swap on references — no pop/push,
            // no size-field thrash, no temporaries beyond the swap itself.
            if (context.stackManager->size() < 2) {
                throw errors::RuntimeException("Stack underflow: SWAP requires 2 values");
            }
            auto& top    = context.stackManager->peekRef(0);
            auto& second = context.stackManager->peekRef(1);
            std::swap(top, second);
        }

    private:
        ExecutionContext& context;
    };
}
