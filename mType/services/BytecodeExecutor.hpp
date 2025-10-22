#pragma once
#include "../value/ValueType.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"
#include <memory>

namespace vm::runtime
{
    class VirtualMachine;
}

namespace services
{
    /**
     * Utility class for executing bytecode programs with EventLoop support
     * Encapsulates the logic for deciding between direct execution and EventLoop-based execution
     *
     * Following the JavaScript model:
     * - No await → Direct execution (faster, better error handling)
     * - Has await → EventLoop execution (enables cooperative multitasking)
     */
    class BytecodeExecutor
    {
    public:
        /**
         * Execute a bytecode program with appropriate execution mode
         *
         * @param vm The virtual machine to execute the program on
         * @param program The bytecode program to execute
         * @return The result value from program execution
         * @throws std::runtime_error if execution fails
         */
        static value::Value executeProgram(
            std::shared_ptr<vm::runtime::VirtualMachine> vm,
            const vm::bytecode::BytecodeProgram& program);
    };
}
