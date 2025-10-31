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
}
