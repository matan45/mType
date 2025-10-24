#include "ConstantFoldingPattern.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../analysis/ControlFlowAnalyzer.hpp"

namespace vm::optimization::patterns
{
    using namespace bytecode;

    bool ConstantFoldingPattern::matches(const BytecodeProgram& program,
                                         size_t offset,
                                         const analysis::ControlFlowAnalyzer& cfg) const
    {
        return matchesBinaryInt(program, offset, cfg) ||
               matchesBinaryFloat(program, offset, cfg) ||
               matchesUnary(program, offset, cfg) ||
               matchesComparison(program, offset, cfg) ||
               matchesLogical(program, offset, cfg);
    }

    OptimizationPattern::Replacement ConstantFoldingPattern::apply(const BytecodeProgram& program,
                                                                    size_t offset) const
    {
        // Note: matches() has already been called, so we know one of these will match
        // We check which one by examining the instructions directly

        if (offset + 2 >= program.getInstructionCount())
        {
            return Replacement(0);
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);
        const auto& i3 = program.getInstruction(offset + 2);

        // Binary int
        if (i1.opcode == OpCode::PUSH_INT && i2.opcode == OpCode::PUSH_INT &&
            (i3.opcode == OpCode::ADD_INT || i3.opcode == OpCode::SUB_INT ||
             i3.opcode == OpCode::MUL_INT || i3.opcode == OpCode::DIV_INT || i3.opcode == OpCode::MOD))
        {
            return applyBinaryInt(program, offset);
        }

        // Binary float
        if (i1.opcode == OpCode::PUSH_FLOAT && i2.opcode == OpCode::PUSH_FLOAT &&
            (i3.opcode == OpCode::ADD_FLOAT || i3.opcode == OpCode::SUB_FLOAT ||
             i3.opcode == OpCode::MUL_FLOAT || i3.opcode == OpCode::DIV_FLOAT))
        {
            return applyBinaryFloat(program, offset);
        }

        // Comparison
        if (i1.opcode == OpCode::PUSH_INT && i2.opcode == OpCode::PUSH_INT &&
            (i3.opcode == OpCode::EQ_INT || i3.opcode == OpCode::NE_INT ||
             i3.opcode == OpCode::LT_INT || i3.opcode == OpCode::GT_INT))
        {
            return applyComparison(program, offset);
        }

        // Logical
        if (i1.opcode == OpCode::PUSH_BOOL && i2.opcode == OpCode::PUSH_BOOL &&
            (i3.opcode == OpCode::AND || i3.opcode == OpCode::OR))
        {
            return applyLogical(program, offset);
        }

        // Unary (only needs 2 instructions)
        if (offset + 1 < program.getInstructionCount())
        {
            const auto& i2_unary = program.getInstruction(offset + 1);
            if ((i1.opcode == OpCode::PUSH_INT || i1.opcode == OpCode::PUSH_BOOL) &&
                (i2_unary.opcode == OpCode::NEG || i2_unary.opcode == OpCode::NOT))
            {
                return applyUnary(program, offset);
            }
        }

        return Replacement(0);
    }

    // === Binary Integer Operations ===

    bool ConstantFoldingPattern::matchesBinaryInt(const BytecodeProgram& program,
                                                   size_t offset,
                                                   const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + 2 >= program.getInstructionCount())
        {
            return false;
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);
        const auto& i3 = program.getInstruction(offset + 2);

        // Match: PUSH_INT, PUSH_INT, <INT_OP>
        if (i1.opcode != OpCode::PUSH_INT || i2.opcode != OpCode::PUSH_INT)
        {
            return false;
        }

        if (i3.opcode != OpCode::ADD_INT &&
            i3.opcode != OpCode::SUB_INT &&
            i3.opcode != OpCode::MUL_INT &&
            i3.opcode != OpCode::DIV_INT &&
            i3.opcode != OpCode::MOD)
        {
            return false;
        }

        // Ensure no jump targets in the middle
        return cfg.canOptimizeRange(offset, offset + 3);
    }

    OptimizationPattern::Replacement ConstantFoldingPattern::applyBinaryInt(const BytecodeProgram& program,
                                                                             size_t offset) const
    {
        const auto& pool = program.getConstantPool();
        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);
        const auto& i3 = program.getInstruction(offset + 2);

        int val1 = pool.getInteger(i1.operands[0]);
        int val2 = pool.getInteger(i2.operands[0]);

        int result = performIntOp(i3.opcode, val1, val2);

        // Add result to constant pool
        auto& mutablePool = const_cast<BytecodeProgram&>(program).getConstantPool();
        size_t resultIdx = mutablePool.addInteger(result);

        Replacement rep(3);
        rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::PUSH_INT, static_cast<uint32_t>(resultIdx)));

        return rep;
    }

    // === Binary Float Operations ===

    bool ConstantFoldingPattern::matchesBinaryFloat(const BytecodeProgram& program,
                                                     size_t offset,
                                                     const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + 2 >= program.getInstructionCount())
        {
            return false;
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);
        const auto& i3 = program.getInstruction(offset + 2);

        // Match: PUSH_FLOAT, PUSH_FLOAT, <FLOAT_OP>
        if (i1.opcode != OpCode::PUSH_FLOAT || i2.opcode != OpCode::PUSH_FLOAT)
        {
            return false;
        }

        if (i3.opcode != OpCode::ADD_FLOAT &&
            i3.opcode != OpCode::SUB_FLOAT &&
            i3.opcode != OpCode::MUL_FLOAT &&
            i3.opcode != OpCode::DIV_FLOAT)
        {
            return false;
        }

        return cfg.canOptimizeRange(offset, offset + 3);
    }

    OptimizationPattern::Replacement ConstantFoldingPattern::applyBinaryFloat(const BytecodeProgram& program,
                                                                               size_t offset) const
    {
        const auto& pool = program.getConstantPool();
        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);
        const auto& i3 = program.getInstruction(offset + 2);

        double val1 = pool.getFloat(i1.operands[0]);
        double val2 = pool.getFloat(i2.operands[0]);

        double result = performFloatOp(i3.opcode, val1, val2);

        auto& mutablePool = const_cast<BytecodeProgram&>(program).getConstantPool();
        size_t resultIdx = mutablePool.addFloat(result);

        Replacement rep(3);
        rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::PUSH_FLOAT, static_cast<uint32_t>(resultIdx)));

        return rep;
    }

    // === Unary Operations ===

    bool ConstantFoldingPattern::matchesUnary(const BytecodeProgram& program,
                                              size_t offset,
                                              const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + 1 >= program.getInstructionCount())
        {
            return false;
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);

        // Match: PUSH_INT/PUSH_BOOL, NEG/NOT
        bool validPush = (i1.opcode == OpCode::PUSH_INT || i1.opcode == OpCode::PUSH_BOOL);
        bool validUnary = (i2.opcode == OpCode::NEG || i2.opcode == OpCode::NOT);

        if (!validPush || !validUnary)
        {
            return false;
        }

        // NOT only works on booleans
        if (i2.opcode == OpCode::NOT && i1.opcode != OpCode::PUSH_BOOL)
        {
            return false;
        }

        return cfg.canOptimizeRange(offset, offset + 2);
    }

    OptimizationPattern::Replacement ConstantFoldingPattern::applyUnary(const BytecodeProgram& program,
                                                                         size_t offset) const
    {
        const auto& pool = program.getConstantPool();
        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 2);

        Replacement rep(2);

        if (i2.opcode == OpCode::NEG)
        {
            int val = pool.getInteger(i1.operands[0]);
            int result = -val;

            auto& mutablePool = const_cast<BytecodeProgram&>(program).getConstantPool();
            size_t resultIdx = mutablePool.addInteger(result);
            rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::PUSH_INT, static_cast<uint32_t>(resultIdx)));
        }
        else if (i2.opcode == OpCode::NOT)
        {
            int val = pool.getInteger(i1.operands[0]);  // Boolean stored as int
            int result = !val;

            auto& mutablePool = const_cast<BytecodeProgram&>(program).getConstantPool();
            size_t resultIdx = mutablePool.addInteger(result);
            rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::PUSH_BOOL, static_cast<uint32_t>(resultIdx)));
        }

        return rep;
    }

    // === Comparison Operations ===

    bool ConstantFoldingPattern::matchesComparison(const BytecodeProgram& program,
                                                    size_t offset,
                                                    const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + 2 >= program.getInstructionCount())
        {
            return false;
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);
        const auto& i3 = program.getInstruction(offset + 2);

        if (i1.opcode != OpCode::PUSH_INT || i2.opcode != OpCode::PUSH_INT)
        {
            return false;
        }

        if (i3.opcode != OpCode::EQ_INT &&
            i3.opcode != OpCode::NE_INT &&
            i3.opcode != OpCode::LT_INT &&
            i3.opcode != OpCode::GT_INT)
        {
            return false;
        }

        return cfg.canOptimizeRange(offset, offset + 3);
    }

    OptimizationPattern::Replacement ConstantFoldingPattern::applyComparison(const BytecodeProgram& program,
                                                                              size_t offset) const
    {
        const auto& pool = program.getConstantPool();
        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);
        const auto& i3 = program.getInstruction(offset + 2);

        int val1 = pool.getInteger(i1.operands[0]);
        int val2 = pool.getInteger(i2.operands[0]);

        bool result = performComparison(i3.opcode, val1, val2);

        auto& mutablePool = const_cast<BytecodeProgram&>(program).getConstantPool();
        size_t resultIdx = mutablePool.addInteger(result ? 1 : 0);

        Replacement rep(3);
        rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::PUSH_BOOL, static_cast<uint32_t>(resultIdx)));

        return rep;
    }

    // === Logical Operations ===

    bool ConstantFoldingPattern::matchesLogical(const BytecodeProgram& program,
                                                 size_t offset,
                                                 const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + 2 >= program.getInstructionCount())
        {
            return false;
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);
        const auto& i3 = program.getInstruction(offset + 2);

        if (i1.opcode != OpCode::PUSH_BOOL || i2.opcode != OpCode::PUSH_BOOL)
        {
            return false;
        }

        if (i3.opcode != OpCode::AND && i3.opcode != OpCode::OR)
        {
            return false;
        }

        return cfg.canOptimizeRange(offset, offset + 3);
    }

    OptimizationPattern::Replacement ConstantFoldingPattern::applyLogical(const BytecodeProgram& program,
                                                                           size_t offset) const
    {
        const auto& pool = program.getConstantPool();
        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);
        const auto& i3 = program.getInstruction(offset + 2);

        bool val1 = pool.getInteger(i1.operands[0]) != 0;
        bool val2 = pool.getInteger(i2.operands[0]) != 0;

        bool result;
        if (i3.opcode == OpCode::AND)
        {
            result = val1 && val2;
        }
        else  // OR
        {
            result = val1 || val2;
        }

        auto& mutablePool = const_cast<BytecodeProgram&>(program).getConstantPool();
        size_t resultIdx = mutablePool.addInteger(result ? 1 : 0);

        Replacement rep(3);
        rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::PUSH_BOOL, static_cast<uint32_t>(resultIdx)));

        return rep;
    }

    // === Helper Methods ===

    int ConstantFoldingPattern::performIntOp(OpCode op, int a, int b) const
    {
        switch (op)
        {
            case OpCode::ADD_INT: return a + b;
            case OpCode::SUB_INT: return a - b;
            case OpCode::MUL_INT: return a * b;
            case OpCode::DIV_INT: return b != 0 ? a / b : 0;  // Avoid division by zero
            case OpCode::MOD: return b != 0 ? a % b : 0;
            default: return 0;
        }
    }

    double ConstantFoldingPattern::performFloatOp(OpCode op, double a, double b) const
    {
        switch (op)
        {
            case OpCode::ADD_FLOAT: return a + b;
            case OpCode::SUB_FLOAT: return a - b;
            case OpCode::MUL_FLOAT: return a * b;
            case OpCode::DIV_FLOAT: return b != 0.0 ? a / b : 0.0;
            default: return 0.0;
        }
    }

    bool ConstantFoldingPattern::performComparison(OpCode op, int a, int b) const
    {
        switch (op)
        {
            case OpCode::EQ_INT: return a == b;
            case OpCode::NE_INT: return a != b;
            case OpCode::LT_INT: return a < b;
            case OpCode::GT_INT: return a > b;
            default: return false;
        }
    }

} // namespace vm::optimization::patterns
