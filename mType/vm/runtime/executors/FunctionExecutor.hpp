#pragma once
#include <span>
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/AccessViolationException.hpp"
#include "../../../ast/AccessModifier.hpp"

namespace vm::runtime
{
    /**
     * Executes function call opcodes
     * Handles CALL, CALL_STATIC
     * Manages call frames, argument passing, and static method access validation
     */
    class FunctionExecutor
    {
    public:
        explicit FunctionExecutor(ExecutionContext& ctx);
        ~FunctionExecutor() = default;

        // Function call operations
        void handleCall(const bytecode::BytecodeProgram::Instruction& instr);
        void handleCallFast(const bytecode::BytecodeProgram::Instruction& instr);
        void handleCallStatic(const bytecode::BytecodeProgram::Instruction& instr);

        // Public helper for lambda-to-interface conversion (used by ObjectExecutor too).
        // Mutates arg slots in place; does not resize. Accepts std::span<Value> so
        // callers can pass a SmallArgsBuffer::span() or a std::vector implicitly.
        void convertLambdaArgumentsToInterfaces(
            std::span<value::Value> args,
            const std::vector<std::string>& parameterTypes
        );

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
