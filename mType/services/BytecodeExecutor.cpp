#include "BytecodeExecutor.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../runtime/EventLoop.hpp"
#include <stdexcept>
#include "../value/PromiseValue.hpp"
#include <iostream>

namespace services
{
    value::Value BytecodeExecutor::executeProgram(
        std::shared_ptr<vm::runtime::VirtualMachine> vm,
        const vm::bytecode::BytecodeProgram& program)
    {
        // JavaScript model: Only use EventLoop if program actually contains await
        // - No await → Direct execution (faster, better error handling)
        // - Has await → EventLoop execution (enables cooperative multitasking)
        if (program.hasAwaitInstructions())
        {
            // Program uses await - need EventLoop for task suspension/resumption
            auto* eventLoop = vm->ensureEventLoop();

            // Schedule main program as a task
            // Capture program by reference to avoid expensive copy
            // Safe because eventLoop->run() is synchronous and program stays in scope
            size_t mainTaskId = eventLoop->scheduleTask(
                [vm, &program]() -> value::Value {
                    return vm->execute(program);
                }
            );

            // Set VM reference so it knows its task ID
            // VM is owned by shared_ptr which supports enable_shared_from_this
            eventLoop->setTaskVM(mainTaskId, vm);

            // Hold a reference to the main task BEFORE run(), because
            // cleanupCompletedTasks() inside run() removes failed/completed tasks
            // from allTasks, which would make getTask() return nullptr after run().
            auto mainTask = eventLoop->getTask(mainTaskId);

            // Run event loop until all tasks complete
            eventLoop->run();

            // Check if main task failed and re-throw error
            if (mainTask && mainTask->state == ::runtime::TaskState::FAILED)
            {
                throw std::runtime_error(mainTask->errorMessage);
            }

            // Return the result from the main task's promise
            if (mainTask && mainTask->state == ::runtime::TaskState::COMPLETED && mainTask->resultPromise)
            {
                if (mainTask->resultPromise->isFulfilled())
                {
                    return mainTask->resultPromise->getValue();
                }
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
