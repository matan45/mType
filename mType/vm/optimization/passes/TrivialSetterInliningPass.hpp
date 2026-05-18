#pragma once

#include "../base/BytecodePass.hpp"

namespace vm::optimization::passes
{
    /**
     * Rewrites CALL_METHOD instructions targeting a trivial setter
     *
     *   set X(v) { this.X = v; }
     *
     * directly into INLINE_SET_FIELD, skipping the call-frame setup. Only
     * applies when the setter is provably not overridden by any subclass
     * (the field-index baked into INLINE_SET_FIELD is a static binding).
     *
     * Operates on BytecodeProgram metadata + instruction stream only — no
     * AST or environment dependency. Extracted from
     * BytecodeCompiler.cpp::inlineTrivialSetters.
     */
    class TrivialSetterInliningPass : public base::BytecodePass
    {
    public:
        TrivialSetterInliningPass();

        void optimize(bytecode::BytecodeProgram& program,
                      base::BytecodeOptimizationContext& context) override;

        std::string getDescription() const override
        {
            return "Rewrite calls to trivial setters as INLINE_SET_FIELD";
        }
    };
}
