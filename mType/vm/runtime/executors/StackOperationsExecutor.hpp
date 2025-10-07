#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace vm::runtime
{
    /**
     * Executes stack manipulation opcodes
     * Handles PUSH_*, POP, DUP, SWAP operations
     */
    class StackOperationsExecutor
    {
    public:
        explicit StackOperationsExecutor(ExecutionContext& ctx);
        ~StackOperationsExecutor() = default;

        // Stack push operations
        void handlePushInt(const bytecode::BytecodeProgram::Instruction& instr);
        void handlePushFloat(const bytecode::BytecodeProgram::Instruction& instr);
        void handlePushString(const bytecode::BytecodeProgram::Instruction& instr);
        void handlePushBool(const bytecode::BytecodeProgram::Instruction& instr);
        void handlePushNull();

        // Stack manipulation operations
        void handlePop();
        void handleDup();
        void handleSwap();

    private:
        ExecutionContext& context;
    };
}
