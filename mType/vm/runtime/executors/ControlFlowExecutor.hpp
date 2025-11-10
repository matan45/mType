#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace vm::runtime
{
    /**
     * Executes control flow opcodes
     * Handles JUMP, JUMP_IF_FALSE, JUMP_IF_TRUE, JUMP_BACK, RETURN, RETURN_VALUE
     */
    class ControlFlowExecutor
    {
    public:
        explicit ControlFlowExecutor(ExecutionContext& ctx);
        ~ControlFlowExecutor() = default;

        // Jump operations
        void handleJump(const bytecode::BytecodeProgram::Instruction& instr);
        void handleJumpIfFalse(const bytecode::BytecodeProgram::Instruction& instr);
        void handleJumpIfTrue(const bytecode::BytecodeProgram::Instruction& instr);
        void handleJumpIfFalseOrPop(const bytecode::BytecodeProgram::Instruction& instr);
        void handleJumpIfTrueOrPop(const bytecode::BytecodeProgram::Instruction& instr);
        void handleJumpBack(const bytecode::BytecodeProgram::Instruction& instr);

        // Return operations
        void handleReturn();
        void handleReturnValue();

    private:
        ExecutionContext& context;

        // Helper method for truthiness evaluation
        bool isTruthy(const value::Value& val) const;
    };
}
