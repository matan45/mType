#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/AccessViolationException.hpp"
#include "../../../ast/AccessModifier.hpp"

namespace vm::runtime
{
    /**
     * Executes function call opcodes
     * Handles CALL, CALL_NATIVE, CALL_STATIC
     * Manages call frames, argument passing, and static method access validation
     */
    class FunctionExecutor
    {
    public:
        explicit FunctionExecutor(ExecutionContext& ctx);
        ~FunctionExecutor() = default;

        // Function call operations
        void handleCall(const bytecode::BytecodeProgram::Instruction& instr);
        void handleCallNative(const bytecode::BytecodeProgram::Instruction& instr);
        void handleCallStatic(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;

        // Helper method for access modifier validation
        void validateStaticMethodAccess(
            const std::string& className,
            const std::string& methodName,
            ast::AccessModifier accessMod
        );
    };
}
