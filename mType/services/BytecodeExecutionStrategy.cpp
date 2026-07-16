#include "BytecodeExecutionStrategy.hpp"
#include "BytecodeExecutor.hpp"
#include "BytecodeProgramBinding.hpp"
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

    BytecodeExecutionStrategy::~BytecodeExecutionStrategy()
    {
        releaseActiveProgram();
    }

    void BytecodeExecutionStrategy::releaseActiveProgram()
    {
        BytecodeProgramBinding::release(
            vm, scriptAPI, activeProgram);
    }

    value::Value BytecodeExecutionStrategy::execute(ast::ASTNode* ast)
    {
        try
        {
            // NOTE: Imports are already resolved in executeScriptAST before optimization
            // No need to resolve them again here

            // Compilation registers/replaces environment ClassDefinitions as a
            // side effect. Invalidate old JIT/IC state before that publication,
            // while activeProgram still owns every referenced metadata object.
            BytecodeProgramBinding::clearCurrent(vm, scriptAPI);

            // Compile AST to bytecode. activeProgram remains alive until the
            // fully-constructed replacement is committed below.
            auto nextProgram =
                std::make_unique<vm::bytecode::BytecodeProgram>(
                    compiler->compile(ast));

            BytecodeProgramBinding::replace(
                vm, scriptAPI, activeProgram, std::move(nextProgram));
            return executeBytecodeProgram(*activeProgram);
        }
        catch (const std::exception&)
        {
            throw;
        }
    }

    value::Value BytecodeExecutionStrategy::executeBytecodeProgram(const vm::bytecode::BytecodeProgram& program)
    {
        // BytecodeProgramBinding already synchronized VM and ScriptAPI. Leave
        // the completed (or failed) program bound and owned so post-run stats
        // and interop remain available until the next coordinated replacement.
        return BytecodeExecutor::executeProgram(vm, program);
    }
}
