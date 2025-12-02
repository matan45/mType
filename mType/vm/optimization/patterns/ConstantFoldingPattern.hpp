#pragma once
#include "../OptimizationPattern.hpp"

namespace vm::optimization::patterns
{
    /**
     * Constant Folding Pattern
     * Evaluates constant expressions at compile-time instead of runtime
     *
     * Examples:
     * - PUSH_INT 5, PUSH_INT 3, ADD_INT -> PUSH_INT 8
     * - PUSH_FLOAT 2.0, PUSH_FLOAT 3.0, MUL_FLOAT -> PUSH_FLOAT 6.0
     * - PUSH_BOOL true, NOT -> PUSH_BOOL false
     *
     * Priority: High (100) - enables other optimizations
     */
    class ConstantFoldingPattern : public OptimizationPattern
    {
    public:
        bool matches(const bytecode::BytecodeProgram& program,
                    size_t offset,
                    const analysis::ControlFlowAnalyzer& cfg) const override;

        Replacement apply(const bytecode::BytecodeProgram& program,
                         size_t offset) const override;

        std::string getName() const override { return "ConstantFolding"; }
        int getPriority() const override { return 100; }

    private:
        // Binary integer operations
        bool matchesBinaryInt(const bytecode::BytecodeProgram& program,
                             size_t offset,
                             const analysis::ControlFlowAnalyzer& cfg) const;

        Replacement applyBinaryInt(const bytecode::BytecodeProgram& program,
                                  size_t offset) const;

        // Binary float operations
        bool matchesBinaryFloat(const bytecode::BytecodeProgram& program,
                               size_t offset,
                               const analysis::ControlFlowAnalyzer& cfg) const;

        Replacement applyBinaryFloat(const bytecode::BytecodeProgram& program,
                                    size_t offset) const;

        // Unary operations
        bool matchesUnary(const bytecode::BytecodeProgram& program,
                         size_t offset,
                         const analysis::ControlFlowAnalyzer& cfg) const;

        Replacement applyUnary(const bytecode::BytecodeProgram& program,
                              size_t offset) const;

        // Comparison operations
        bool matchesComparison(const bytecode::BytecodeProgram& program,
                              size_t offset,
                              const analysis::ControlFlowAnalyzer& cfg) const;

        Replacement applyComparison(const bytecode::BytecodeProgram& program,
                                   size_t offset) const;

        // Logical operations
        bool matchesLogical(const bytecode::BytecodeProgram& program,
                           size_t offset,
                           const analysis::ControlFlowAnalyzer& cfg) const;

        Replacement applyLogical(const bytecode::BytecodeProgram& program,
                                size_t offset) const;

        // Helper: Perform integer operation
        int64_t performIntOp(bytecode::OpCode op, int64_t a, int64_t b) const;

        // Helper: Perform float operation
        double performFloatOp(bytecode::OpCode op, double a, double b) const;

        // Helper: Perform comparison
        bool performComparison(bytecode::OpCode op, int64_t a, int64_t b) const;
    };

} // namespace vm::optimization::patterns
