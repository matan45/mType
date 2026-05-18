#pragma once

#include "../base/BytecodePass.hpp"

namespace vm::optimization::passes
{
    /**
     * Rewrites CALL_METHOD instructions targeting a trivial getter
     *
     *   get X() { return this.X; }
     *
     * directly into INLINE_GET_FIELD. Skipped when the field is
     * private/protected (the inline opcode bypasses access validation) or
     * when any subclass overrides the method.
     *
     * Extracted from BytecodeCompiler.cpp::inlineTrivialGetters.
     */
    class TrivialGetterInliningPass : public base::BytecodePass
    {
    public:
        TrivialGetterInliningPass();

        void optimize(bytecode::BytecodeProgram& program,
                      base::BytecodeOptimizationContext& context) override;

        std::string getDescription() const override
        {
            return "Rewrite calls to trivial getters as INLINE_GET_FIELD";
        }
    };
}
