#pragma once
#include "../OptimizationPattern.hpp"
#include <cstddef>

namespace vm::optimization::patterns
{
    /**
     * Type Specialization Pattern
     * Promotes generic operations to type-specialized versions
     *
     * Examples:
     * - PUSH_INT, PUSH_INT, ADD → PUSH_INT, PUSH_INT, ADD_INT
     * - PUSH_FLOAT, PUSH_FLOAT, MUL → PUSH_FLOAT, PUSH_FLOAT, MUL_FLOAT
     * - PUSH_INT, PUSH_INT, EQ → PUSH_INT, PUSH_INT, EQ_INT
     *
     * Priority: Low (30) - minor performance gain
     */
    class TypeSpecializationPattern : public OptimizationPattern
    {
    public:
        bool matches(const bytecode::BytecodeProgram& program,
                    size_t offset,
                    const analysis::ControlFlowAnalyzer& cfg) const override;

        Replacement apply(const bytecode::BytecodeProgram& program,
                         size_t offset) const override;

        std::string getName() const override { return "TypeSpecialization"; }
        int getPriority() const override { return 30; }

    private:
        // Get the specialized version of a generic operation
        bytecode::OpCode getSpecializedOpCode(bytecode::OpCode genericOp,
                                              bytecode::OpCode pushType) const;

        // Check if an opcode is a generic arithmetic operation
        bool isGenericArithmetic(bytecode::OpCode opcode) const;

        // Check if an opcode is a generic comparison operation
        bool isGenericComparison(bytecode::OpCode opcode) const;
    };

} // namespace vm::optimization::patterns
