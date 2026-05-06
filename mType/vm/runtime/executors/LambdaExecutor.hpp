#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace vm::runtime
{
    /**
     * Executes lambda-related opcodes
     * Handles LAMBDA (lambda value creation). Lambda *invocation* flows
     * through CALL_METHOD with the lambda value as receiver — the legacy
     * LAMBDA_INVOKE opcode was removed in MYT-B1 because the compiler
     * never emitted it.
     */
    class LambdaExecutor
    {
    public:
        explicit LambdaExecutor(ExecutionContext& ctx);
        ~LambdaExecutor() = default;

        void handleLambda(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;
    };
}
