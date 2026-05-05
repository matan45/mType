#pragma once
#include "../OptimizationPattern.hpp"
#include <cstddef>

namespace vm::optimization::patterns
{
    /**
     * Superinstruction Fusion Pattern (MYT-202)
     *
     * Fuses adjacent opcode pairs/triples found to dominate interpreter
     * dispatch cost on hot loops into single compile-time "superinstructions".
     * Each fusion shrinks the instruction vector, cutting dispatch cycles
     * + operand decodes for the sequence.
     *
     * Fused sequences:
     *   LOAD_LOCAL s1, LOAD_LOCAL s2, ADD_INT   → LOAD_LOAD_ADD_INT  [s1, s2]
     *   LOAD_LOCAL s1, LOAD_LOCAL s2, SUB_INT   → LOAD_LOAD_SUB_INT  [s1, s2]
     *   LOAD_LOCAL s1, LOAD_LOCAL s2, MUL_INT   → LOAD_LOAD_MUL_INT  [s1, s2]
     *   LOAD_LOCAL s,  GET_FIELD name           → LOAD_GET_FIELD     [s, name]
     *   LOAD_LOCAL src, STORE_LOCAL dst         → LOAD_STORE_LOCAL   [src, dst]
     *   ADD_INT, STORE_LOCAL dst                → ADD_INT_STORE_LOCAL [dst]
     *   OBJECT_TO_VALUE, CREATE_PROMISE         → OBJECT_TO_VALUE_CREATE_PROMISE
     *   CREATE_PROMISE, RETURN_VALUE            → CREATE_PROMISE_RETURN_VALUE
     *
     * Safety: rejects when a fused interior offset is a jump/try/function
     * entry target (via BytecodeProgram::isFusionUnsafeTarget), when any
     * candidate slot is a structural/meta op, or when the site has existing
     * runtime-fused side-table state (defensive — should not fire at compile).
     *
     * Priority: Low (5). Runs after other reductions converge so we fuse
     * the final, settled instruction sequence.
     */
    class SuperinstructionPattern : public OptimizationPattern
    {
    public:
        bool matches(const bytecode::BytecodeProgram& program,
                    size_t offset,
                    const analysis::ControlFlowAnalyzer& cfg) const override;

        Replacement apply(const bytecode::BytecodeProgram& program,
                         size_t offset) const override;

        std::string getName() const override { return "SuperinstructionFusion"; }
        int getPriority() const override { return 5; }
        std::string getDescription() const override
        {
            return "MYT-202: fuse hot opcode pairs/triples into compile-time superinstructions";
        }

    private:
        // Classifies the fusion pattern at offset, if any.
        enum class Kind
        {
            None,
            LoadLoadAddInt,
            LoadLoadSubInt,
            LoadLoadMulInt,
            LoadGetField,
            LoadStoreLocal,
            AddIntStoreLocal,
            ObjectToValueCreatePromise,
            CreatePromiseReturnValue,
        };

        Kind classify(const bytecode::BytecodeProgram& program, size_t offset) const;

        // Reject meta/structural opcodes that must never be fused across.
        static bool isStructural(bytecode::OpCode op);
    };

} // namespace vm::optimization::patterns
