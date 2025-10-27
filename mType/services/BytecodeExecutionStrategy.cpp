#include "BytecodeExecutionStrategy.hpp"
#include "BytecodeExecutor.hpp"
#include "ImportResolver.hpp"
#include "ScriptAPI.hpp"
#include "../vm/compiler/BytecodeCompiler.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include <stdexcept>

namespace services
{
    BytecodeExecutionStrategy::BytecodeExecutionStrategy(vm::compiler::BytecodeCompiler* comp,
                                                         std::shared_ptr<vm::runtime::VirtualMachine> virtualMachine,
                                                         ImportResolver* resolver,
                                                         ScriptAPI* api)
        : compiler(comp), vm(virtualMachine), importResolver(resolver), scriptAPI(api)
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
        // Set bytecode program on ScriptAPI for C++ interop
        if (scriptAPI)
        {
            scriptAPI->setBytecodeProgram(&program);
        }

        try
        {
            // Delegate to BytecodeExecutor utility for consistent execution logic
            return BytecodeExecutor::executeProgram(vm, program);
        }
        catch (...)
        {
            // Clear program reference before rethrowing
            if (scriptAPI)
            {
                scriptAPI->setBytecodeProgram(nullptr);
            }
            throw;
        }

        // Note: No need to clear here since program stays in scope during execution
        // and will be automatically cleared when the next script runs
    }
}
