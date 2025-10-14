#pragma once

namespace constants
{
    /**
     * Execution mode for the interpreter
     * Determines whether to use AST evaluation or bytecode compilation
     */
    enum class ExecutionMode
    {
        /**
         * AST-walking interpreter (original evaluator)
         * Compatible with all language features
         */
        AST_INTERPRETER,

        /**
         * Bytecode compiler + VM execution
         * Higher performance for supported features
         */
        BYTECODE_VM,

        /**
         * Dual validation mode
         * Runs both AST and bytecode and compares results
         * Used for testing and validation
         */
        DUAL_VALIDATION
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
