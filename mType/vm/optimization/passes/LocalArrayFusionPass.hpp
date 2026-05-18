#pragma once

#include "../base/BytecodePass.hpp"

namespace vm::optimization::passes
{
    /**
     * Fuses LOAD_LOCAL + array-op pairs into the corresponding
     * ARRAY_*_LOCAL opcode forms, saving one stack push/pop per access in
     * tight loops over local-rooted arrays.
     *
     * Patterns rewritten:
     *   - LOAD_LOCAL X, ARRAY_LENGTH                        → ARRAY_LENGTH_LOCAL X
     *   - LOAD_LOCAL X, LOAD_LOCAL Y, ARRAY_GET_INT         → ARRAY_GET_INT_LOCAL X (idx via Y)
     *   - LOAD_LOCAL X, LOAD_LOCAL Y, <val>, ARRAY_SET_INT  → ARRAY_SET_INT_LOCAL X
     *   - ARRAY_GET, STORE_LOCAL (MYT-303 alias hint)       → ARRAY_GET_ALIAS, STORE_LOCAL
     *
     * Extracted from BytecodeCompiler.cpp::fuseLocalArrayOps.
     */
    class LocalArrayFusionPass : public base::BytecodePass
    {
    public:
        LocalArrayFusionPass();

        void optimize(bytecode::BytecodeProgram& program,
                      base::BytecodeOptimizationContext& context) override;

        std::string getDescription() const override
        {
            return "Fuse LOAD_LOCAL + array ops into ARRAY_*_LOCAL variants";
        }
    };
}
