#pragma once
#include "ExecutionStrategy.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"
#include <memory>

namespace vm::compiler
{
    class BytecodeCompiler;
}

namespace vm::runtime
{
    class VirtualMachine;
}

namespace services
{
    class ImportResolver;

    /**
     * Execution strategy for bytecode VM mode
     * Compiles AST to bytecode and executes on virtual machine
     */
    class BytecodeExecutionStrategy : public ExecutionStrategy
    {
    private:
        vm::compiler::BytecodeCompiler* compiler;
        std::shared_ptr<vm::runtime::VirtualMachine> vm;
        ImportResolver* importResolver;

        // Helper for executing compiled bytecode program
        value::Value executeBytecodeProgram(const vm::bytecode::BytecodeProgram& program);

    public:
        BytecodeExecutionStrategy(vm::compiler::BytecodeCompiler* comp,
                                 std::shared_ptr<vm::runtime::VirtualMachine> virtualMachine,
                                 ImportResolver* resolver);
        ~BytecodeExecutionStrategy() override = default;

        value::Value execute(ast::ASTNode* ast) override;
    };
}
