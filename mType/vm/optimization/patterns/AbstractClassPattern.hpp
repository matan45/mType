#pragma once
#include "../OptimizationPattern.hpp"
#include "../../bytecode/BytecodeProgram.hpp"

namespace vm::optimization::patterns
{
    /**
     * Abstract Class Validation Pattern
     * Validates abstract class usage at bytecode level
     *
     * Detects:
     * - NEW_OBJECT instructions attempting to instantiate abstract classes
     * - Provides compile-time warnings for abstract class violations
     *
     * This is primarily a validation pattern rather than an optimization,
     * but it helps catch abstract class instantiation errors during
     * bytecode compilation before runtime.
     *
     * Pattern detected:
     *   PUSH_STRING <className>  (abstract class name)
     *   [push constructor args...]
     *   NEW_OBJECT <argCount>
     *
     * Action: Generate warning (not a transformation, validation only)
     *
     * Priority: Low (20) - runs after other optimizations
     */
    class AbstractClassPattern : public OptimizationPattern
    {
    private:
        const bytecode::BytecodeProgram* programRef;

    public:
        AbstractClassPattern();

        bool matches(const bytecode::BytecodeProgram& program,
                    size_t offset,
                    const analysis::ControlFlowAnalyzer& cfg) const override;

        Replacement apply(const bytecode::BytecodeProgram& program,
                         size_t offset) const override;

        std::string getName() const override { return "AbstractClassValidation"; }
        int getPriority() const override { return 20; }

    private:
        // Check if a class is abstract by looking up in class metadata
        bool isAbstractClass(const bytecode::BytecodeProgram& program,
                           const std::string& className) const;

        // Extract class name from PUSH_STRING instruction before NEW_OBJECT
        std::string extractClassNameBeforeNew(const bytecode::BytecodeProgram& program,
                                              size_t newOffset) const;
    };

} // namespace vm::optimization::patterns
