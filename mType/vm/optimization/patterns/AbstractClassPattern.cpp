#include "AbstractClassPattern.hpp"
#include "../../bytecode/OpCode.hpp"

namespace vm::optimization::patterns
{
    AbstractClassPattern::AbstractClassPattern()
        : programRef(nullptr)
    {
    }

    bool AbstractClassPattern::isAbstractClass(
        const bytecode::BytecodeProgram& program,
        const std::string& className) const
    {
        // Look up class in bytecode program's class metadata
        const auto& classes = program.getClasses();

        for (const auto& classMetadata : classes) {
            if (classMetadata.name == className) {
                return classMetadata.isAbstract;
            }
        }

        // Class not found in metadata, assume it's not abstract
        return false;
    }

    std::string AbstractClassPattern::extractClassNameBeforeNew(
        const bytecode::BytecodeProgram& program,
        size_t newOffset) const
    {
        // NEW_OBJECT is typically preceded by:
        // PUSH_STRING <className> or LOAD_CONST <stringIndex>
        // [push constructor arguments...]
        // NEW_OBJECT <argCount>

        // We need to trace back to find the class name
        // This is a simplified heuristic - may need refinement

        if (newOffset == 0) {
            return "";
        }

        const auto& instructions = program.getInstructions();
        const auto& newInstr = instructions[newOffset];

        // Get argument count from NEW_OBJECT
        if (newInstr.operands.empty()) {
            return "";
        }

        size_t argCount = newInstr.operands[0];

        // Trace back past the constructor arguments
        // Each argument is typically 1 instruction (PUSH_* or LOAD_*)
        // This is simplified - real implementation might need data flow analysis
        size_t classNameOffset = newOffset - argCount - 1;

        if (classNameOffset >= instructions.size()) {
            return "";
        }

        const auto& classNameInstr = instructions[classNameOffset];

        // Check if it's a string constant load
        if (classNameInstr.opcode == bytecode::OpCode::PUSH_STRING) {

            if (!classNameInstr.operands.empty()) {
                size_t stringIndex = classNameInstr.operands[0];
                const auto& constantPool = program.getConstantPool();

                // Safely get string from constant pool
                try {
                    return constantPool.getString(stringIndex);
                } catch (...) {
                    return "";
                }
            }
        }

        return "";
    }

    bool AbstractClassPattern::matches(
        const bytecode::BytecodeProgram& program,
        size_t offset,
        const analysis::ControlFlowAnalyzer& cfg) const
    {
        const auto& instructions = program.getInstructions();

        if (offset >= instructions.size()) {
            return false;
        }

        const auto& instr = instructions[offset];

        // Match NEW_OBJECT instruction
        if (instr.opcode != bytecode::OpCode::NEW_OBJECT) {
            return false;
        }

        // Extract class name
        std::string className = extractClassNameBeforeNew(program, offset);
        if (className.empty()) {
            return false;
        }

        // Check if class is abstract
        return isAbstractClass(program, className);
    }

    OptimizationPattern::Replacement AbstractClassPattern::apply(
        const bytecode::BytecodeProgram& program,
        size_t offset) const
    {
        // This is a validation pattern, not a transformation pattern
        // We don't actually modify the bytecode, we just detect the issue

        std::string className = extractClassNameBeforeNew(program, offset);

        // Return empty replacement (no transformation)
        Replacement replacement;
        replacement.originalLength = 0;  // Don't remove any instructions
        replacement.sourceLocationPreserved = true;

        return replacement;
    }

} // namespace vm::optimization::patterns
