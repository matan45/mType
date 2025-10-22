#include "BytecodeExecutionStrategy.hpp"
#include "BytecodeExecutor.hpp"
#include "ImportResolver.hpp"
#include "../vm/compiler/BytecodeCompiler.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include <stdexcept>

namespace services
{
    BytecodeExecutionStrategy::BytecodeExecutionStrategy(vm::compiler::BytecodeCompiler* comp,
                                                         std::shared_ptr<vm::runtime::VirtualMachine> virtualMachine,
                                                         ImportResolver* resolver)
        : compiler(comp), vm(virtualMachine), importResolver(resolver)
    {
    }

    value::Value BytecodeExecutionStrategy::execute(ast::ASTNode* ast)
    {
        try
        {
            // NOTE: Imports are already resolved in executeScriptAST before optimization
            // No need to resolve them again here

            // Compile AST to bytecode
            // The BytecodeCompiler will register all classes during compilation
            auto program = compiler->compile(ast);

            return executeBytecodeProgram(program);
        }
        catch (const std::exception&)
        {
            throw;
        }
    }

    value::Value BytecodeExecutionStrategy::executeBytecodeProgram(const vm::bytecode::BytecodeProgram& program)
    {
        // Delegate to BytecodeExecutor utility for consistent execution logic
        return BytecodeExecutor::executeProgram(vm, program);
    }
}
