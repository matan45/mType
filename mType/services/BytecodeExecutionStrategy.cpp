#include "BytecodeExecutionStrategy.hpp"
#include "ImportResolver.hpp"
#include "../vm/compiler/BytecodeCompiler.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../runtime/EventLoop.hpp"
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
        // JavaScript model: Only use EventLoop if program actually contains await
        // - No await → Direct execution (faster, better error handling)
        // - Has await → EventLoop execution (enables cooperative multitasking)
        if (program.hasAwaitInstructions())
        {
            // Program uses await - need EventLoop for task suspension/resumption
            auto* eventLoop = vm->ensureEventLoop();

            // Schedule main program as a task
            size_t mainTaskId = eventLoop->scheduleTask(
                [this, program]() -> value::Value {
                    return vm->execute(program);
                }
            );

            // Set VM reference so it knows its task ID
            // VM is owned by shared_ptr which supports enable_shared_from_this
            eventLoop->setTaskVM(mainTaskId, vm);

            // Run event loop until all tasks complete
            eventLoop->run();

            // Check if main task failed and re-throw error
            auto mainTask = eventLoop->getTask(mainTaskId);
            if (mainTask && mainTask->state == ::runtime::TaskState::FAILED)
            {
                throw std::runtime_error(mainTask->errorMessage);
            }

            return std::monostate{};
        }
        else
        {
            // No await in program - execute directly without EventLoop overhead
            return vm->execute(program);
        }
    }
}
