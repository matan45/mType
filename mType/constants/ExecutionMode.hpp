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
     * Optimization level for bytecode compilation
     */
    enum class OptimizationLevel
    {
        O0,  // No optimization - AST interpretation
        O1,  // Basic bytecode compilation
        O2  // Optimized bytecode (constant folding, dead code elimination)
    };
}
