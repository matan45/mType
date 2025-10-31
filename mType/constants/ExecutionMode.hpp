#pragma once

namespace constants
{
    /**
     * Execution mode for the interpreter
     * Currently only bytecode VM is supported
     */
    enum class ExecutionMode
    {
        /**
         * Bytecode compiler + VM execution
         * Default and only execution mode
         */
        BYTECODE_VM
    };

    /**
     * Optimization level for the compiler
     */
    enum class OptimizationLevel
    {
        Debug,   // No optimization - no dead code passes
        Release  // Full optimization - includes dead code elimination and unused declaration removal
    };

    /**
     * Virtual Machine configuration constants
     */
    namespace vm
    {
        /**
         * Default call stack capacity for initial memory reservation
         * This is just an optimization hint - the stack can grow beyond this
         */
        constexpr size_t DEFAULT_CALL_STACK_CAPACITY = 64;

        /**
         * Maximum allowed call stack depth before throwing stack overflow error
         * This limit prevents infinite recursion from consuming all memory
         * Can be overridden via VirtualMachine constructor parameter
         */
        constexpr size_t DEFAULT_MAX_CALL_STACK_SIZE = 1000;
    }
}
