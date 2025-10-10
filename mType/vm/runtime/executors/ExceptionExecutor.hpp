#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/UserException.hpp"

namespace vm::runtime
{
    /**
     * Executes exception handling opcodes
     * Handles TRY_BEGIN, TRY_END, CATCH, THROW, FINALLY
     *
     * Note: Most exception handling logic is implemented using C++ exceptions
     * in the VM's interpretLoop. These handlers primarily manage control flow
     * and marker opcodes.
     */
    class ExceptionExecutor
    {
    public:
        explicit ExceptionExecutor(ExecutionContext& ctx);
        ~ExceptionExecutor() = default;

        // Exception handling operations
        void handleTryBegin(const bytecode::BytecodeProgram::Instruction& instr);
        void handleTryEnd(const bytecode::BytecodeProgram::Instruction& instr);
        void handleCatch(const bytecode::BytecodeProgram::Instruction& instr);
        void handleThrow(const bytecode::BytecodeProgram::Instruction& instr);
        void handleFinally(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;
    };
}
