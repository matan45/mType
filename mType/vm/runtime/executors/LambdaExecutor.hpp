#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace vm::runtime
{
    /**
     * Executes lambda-related opcodes
     * Handles LAMBDA and LAMBDA_INVOKE
     * Manages lambda creation and invocation with closure capture
     */
    class LambdaExecutor
    {
    public:
        explicit LambdaExecutor(ExecutionContext& ctx);
        ~LambdaExecutor() = default;

        // Lambda operations
        void handleLambda(const bytecode::BytecodeProgram::Instruction& instr);
        void handleLambdaInvoke(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;
    };
}
