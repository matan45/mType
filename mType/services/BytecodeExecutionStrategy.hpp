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
    class ScriptAPI;

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
        ScriptAPI* scriptAPI;  // For updating bytecode program reference

        // Helper for executing compiled bytecode program
        value::Value executeBytecodeProgram(const vm::bytecode::BytecodeProgram& program);

    public:
        BytecodeExecutionStrategy(vm::compiler::BytecodeCompiler* comp,
                                 std::shared_ptr<vm::runtime::VirtualMachine> virtualMachine,
                                 ImportResolver* resolver,
                                 ScriptAPI* api = nullptr);
        ~BytecodeExecutionStrategy() override = default;

        value::Value execute(ast::ASTNode* ast) override;
    };
}
