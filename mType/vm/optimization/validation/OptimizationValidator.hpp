#pragma once
#include <vector>
#include <string>
#include "../../bytecode/BytecodeProgram.hpp"
#include "../analysis/ControlFlowAnalyzer.hpp"

namespace vm::optimization::validation
{
    /**
     * Validates bytecode programs for correctness after optimization
     * Ensures optimizations haven't broken the program
     * Single Responsibility: Validation of optimized bytecode
     */
    class OptimizationValidator
    {
    public:
        /**
         * Validation result
         */
        struct ValidationResult
        {
            bool isValid = true;
            std::vector<std::string> errors;
            std::vector<std::string> warnings;

            void addError(const std::string& error);
            void addWarning(const std::string& warning);
            std::string toString() const;
        };

        OptimizationValidator() = default;

        /**
         * Validate a bytecode program
         * @param program The bytecode program to validate
         * @return Validation result with errors/warnings
         */
        ValidationResult validate(const bytecode::BytecodeProgram& program);

        /**
         * Validate that optimization preserved program semantics
         * @param original Original program before optimization
         * @param optimized Optimized program
         * @return Validation result
         */
        ValidationResult validateOptimization(const bytecode::BytecodeProgram& original,
                                              const bytecode::BytecodeProgram& optimized);

    private:
        analysis::ControlFlowAnalyzer cfgAnalyzer;

        /**
         * Validate all jump targets are within bounds
         */
        ValidationResult validateJumpTargets(const bytecode::BytecodeProgram& program);

        /**
         * Validate function metadata consistency
         */
        ValidationResult validateFunctionMetadata(const bytecode::BytecodeProgram& program);

        /**
         * Validate constant pool integrity
         */
        ValidationResult validateConstantPool(const bytecode::BytecodeProgram& program);

        /**
         * Validate stack balance
         */
        ValidationResult validateStackBalance(const bytecode::BytecodeProgram& program);

        /**
         * Check if two programs have equivalent structure
         */
        bool areStructurallyEquivalent(const bytecode::BytecodeProgram& p1,
                                       const bytecode::BytecodeProgram& p2);
    };

} // namespace vm::optimization::validation
