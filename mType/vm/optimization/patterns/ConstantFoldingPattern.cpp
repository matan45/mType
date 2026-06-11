#include "ConstantFoldingPattern.hpp"
#include <cstddef>
#include <cstdint>
#include "PatternSafetyHelper.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../analysis/ControlFlowAnalyzer.hpp"

namespace vm::optimization::patterns
{
    using namespace bytecode;

    namespace
    {
        // Window sizes for the fold patterns: a binary fold consumes
        // (operand1, operand2, op) and a unary fold consumes (operand, op).
        // Used by every bounds check in this file so the magic 1/2 offsets
        // have a name.
        constexpr size_t kBinaryFoldWindow = 3;
        constexpr size_t kUnaryFoldWindow = 2;

        bool readInlineBool(const BytecodeProgram::Instruction& instr)
        {
            return instr.inlineOperands[0] != 0;
        }
    }

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

        if (offset >= program.getInstructionCount())
        {
            return Replacement(0);
        }

        const auto& i1 = program.getInstruction(offset);

        // Unary only needs 2 instructions, so handle it before requiring the
        // 3-instruction binary fold window.
        if (offset + kUnaryFoldWindow <= program.getInstructionCount())
        {
            const auto& i2_unary = program.getInstruction(offset + 1);
            if ((i1.opcode == OpCode::PUSH_INT && i2_unary.opcode == OpCode::NEG) ||
                (i1.opcode == OpCode::PUSH_BOOL && i2_unary.opcode == OpCode::NOT))
            {
                return applyUnary(program, offset);
            }
        }

        if (offset + kBinaryFoldWindow > program.getInstructionCount())
        {
            return Replacement(0);
        }

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

        // Comparison (both specialized and generic)
        if (i1.opcode == OpCode::PUSH_INT && i2.opcode == OpCode::PUSH_INT)
        {
            bool isComparisonOp = (i3.opcode == OpCode::EQ_INT || i3.opcode == OpCode::NE_INT ||
                                  i3.opcode == OpCode::LT_INT || i3.opcode == OpCode::GT_INT ||
                                  i3.opcode == OpCode::EQ || i3.opcode == OpCode::NE ||
                                  i3.opcode == OpCode::LT || i3.opcode == OpCode::GT ||
                                  i3.opcode == OpCode::LE || i3.opcode == OpCode::GE);

            if (isComparisonOp)
            {
                return applyComparison(program, offset);
            }
        }

        // Logical
        if (i1.opcode == OpCode::PUSH_BOOL && i2.opcode == OpCode::PUSH_BOOL &&
            (i3.opcode == OpCode::AND || i3.opcode == OpCode::OR))
        {
            return applyLogical(program, offset);
        }

        return Replacement(0);
    }

    // === Binary Integer Operations ===

    bool ConstantFoldingPattern::matchesBinaryInt(const BytecodeProgram& program,
                                                   size_t offset,
                                                   const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + kBinaryFoldWindow > program.getInstructionCount())
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

        int64_t val1 = PatternSafetyHelper::safeGetInteger(pool, i1, 0, offset);
        int64_t val2 = PatternSafetyHelper::safeGetInteger(pool, i2, 0, offset + 1);

        int64_t result = performIntOp(i3.opcode, val1, val2);

        // Add result to constant pool
        auto& mutablePool = const_cast<BytecodeProgram&>(program).getConstantPool();
        size_t resultIdx = mutablePool.addInteger(result);

        Replacement rep(3);
        rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::PUSH_INT, static_cast<uint64_t>(resultIdx)));

        return rep;
    }

    // === Binary Float Operations ===

    bool ConstantFoldingPattern::matchesBinaryFloat(const BytecodeProgram& program,
                                                     size_t offset,
                                                     const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + kBinaryFoldWindow > program.getInstructionCount())
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

        double val1 = PatternSafetyHelper::safeGetFloat(pool, i1, 0, offset);
        double val2 = PatternSafetyHelper::safeGetFloat(pool, i2, 0, offset + 1);

        double result = performFloatOp(i3.opcode, val1, val2);

        auto& mutablePool = const_cast<BytecodeProgram&>(program).getConstantPool();
        size_t resultIdx = mutablePool.addFloat(result);

        Replacement rep(3);
        rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::PUSH_FLOAT, static_cast<uint64_t>(resultIdx)));

        return rep;
    }

    // === Unary Operations ===

    bool ConstantFoldingPattern::matchesUnary(const BytecodeProgram& program,
                                              size_t offset,
                                              const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + kUnaryFoldWindow > program.getInstructionCount())
        {
            return false;
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);

        // Match only the valid typed unary forms.
        bool validUnary = (i1.opcode == OpCode::PUSH_INT && i2.opcode == OpCode::NEG) ||
                          (i1.opcode == OpCode::PUSH_BOOL && i2.opcode == OpCode::NOT);

        return validUnary && cfg.canOptimizeRange(offset, offset + 2);
    }

    OptimizationPattern::Replacement ConstantFoldingPattern::applyUnary(const BytecodeProgram& program,
                                                                         size_t offset) const
    {
        const auto& pool = program.getConstantPool();
        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);  // Fixed: was offset + 2, should be offset + 1

        Replacement rep(2);

        if (i2.opcode == OpCode::NEG)
        {
            int64_t val = PatternSafetyHelper::safeGetInteger(pool, i1, 0, offset);
            int64_t result = -val;

            auto& mutablePool = const_cast<BytecodeProgram&>(program).getConstantPool();
            size_t resultIdx = mutablePool.addInteger(result);
            rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::PUSH_INT, static_cast<uint64_t>(resultIdx)));
        }
        else if (i2.opcode == OpCode::NOT)
        {
            bool result = !readInlineBool(i1);

            rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::PUSH_BOOL, result ? 1 : 0));
        }

        return rep;
    }

    // === Comparison Operations ===

    bool ConstantFoldingPattern::matchesComparison(const BytecodeProgram& program,
                                                    size_t offset,
                                                    const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + kBinaryFoldWindow > program.getInstructionCount())
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

        // Support both specialized (LT_INT, GT_INT) and generic (LT, GT, LE, GE) comparisons
        bool isSpecialized = (i3.opcode == OpCode::EQ_INT ||
                             i3.opcode == OpCode::NE_INT ||
                             i3.opcode == OpCode::LT_INT ||
                             i3.opcode == OpCode::GT_INT);

        bool isGeneric = (i3.opcode == OpCode::EQ ||
                         i3.opcode == OpCode::NE ||
                         i3.opcode == OpCode::LT ||
                         i3.opcode == OpCode::GT ||
                         i3.opcode == OpCode::LE ||
                         i3.opcode == OpCode::GE);

        if (!isSpecialized && !isGeneric)
        {
            return false;
        }

        // Check that we can safely optimize this range
        // This prevents optimizing across basic block boundaries (jump targets, etc.)
        return cfg.canOptimizeRange(offset, offset + 3);
    }

    OptimizationPattern::Replacement ConstantFoldingPattern::applyComparison(const BytecodeProgram& program,
                                                                              size_t offset) const
    {
        const auto& pool = program.getConstantPool();
        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);
        const auto& i3 = program.getInstruction(offset + 2);

        int64_t val1 = PatternSafetyHelper::safeGetInteger(pool, i1, 0, offset);
        int64_t val2 = PatternSafetyHelper::safeGetInteger(pool, i2, 0, offset + 1);

        bool result = performComparison(i3.opcode, val1, val2);

        Replacement rep(3);
        rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::PUSH_BOOL, result ? 1 : 0));

        return rep;
    }

    // === Logical Operations ===

    bool ConstantFoldingPattern::matchesLogical(const BytecodeProgram& program,
                                                 size_t offset,
                                                 const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + kBinaryFoldWindow > program.getInstructionCount())
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
        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);
        const auto& i3 = program.getInstruction(offset + 2);

        bool val1 = readInlineBool(i1);
        bool val2 = readInlineBool(i2);

        bool result;
        if (i3.opcode == OpCode::AND)
        {
            result = val1 && val2;
        }
        else  // OR
        {
            result = val1 || val2;
        }

        Replacement rep(3);
        rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::PUSH_BOOL, result ? 1 : 0));

        return rep;
    }

    // === Helper Methods ===

    int64_t ConstantFoldingPattern::performIntOp(OpCode op, int64_t a, int64_t b) const
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

    bool ConstantFoldingPattern::performComparison(OpCode op, int64_t a, int64_t b) const
    {
        switch (op)
        {
            // Specialized comparison opcodes
            case OpCode::EQ_INT: return a == b;
            case OpCode::NE_INT: return a != b;
            case OpCode::LT_INT: return a < b;
            case OpCode::GT_INT: return a > b;

            // Generic comparison opcodes
            case OpCode::EQ: return a == b;
            case OpCode::NE: return a != b;
            case OpCode::LT: return a < b;
            case OpCode::GT: return a > b;
            case OpCode::LE: return a <= b;
            case OpCode::GE: return a >= b;

            default: return false;
        }
    }

} // namespace vm::optimization::patterns
