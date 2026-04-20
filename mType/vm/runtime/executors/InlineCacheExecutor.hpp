#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../jit/ic/InlineCacheTable.hpp"
#include "../../jit/ic/InlineCacheTypes.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/NullPointerException.hpp"
#include "../../../errors/FieldNotFoundException.hpp"

namespace vm::runtime
{
    class ObjectExecutor;
    class FunctionExecutor;

    class InlineCacheExecutor
    {
    public:
        explicit InlineCacheExecutor(ExecutionContext& ctx,
                                      vm::jit::ic::InlineCacheTable& icTable);

        void setObjectExecutor(ObjectExecutor* objExec) { objectExecutor = objExec; }
        void setFunctionExecutor(FunctionExecutor* funcExec) { functionExecutor = funcExec; }

        // IC-enabled field access
        void handleGetFieldIC(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSetFieldIC(const bytecode::BytecodeProgram::Instruction& instr);
        void handleInlineSetFieldIC(const bytecode::BytecodeProgram::Instruction& instr);
        void handleInlineGetFieldIC(const bytecode::BytecodeProgram::Instruction& instr);

        // IC-enabled method dispatch
        void handleCallMethodIC(const bytecode::BytecodeProgram::Instruction& instr);

        // MYT-173: CALL_METHOD_CACHED fast path. Reads the cached target embedded
        // on the Instruction, does a single shape compare against cachedShape, and
        // dispatches directly (no IC hashmap probe, no per-entry linear scan). On
        // shape miss, rewrites the opcode back to CALL_METHOD and re-enters the
        // generic IC path. Promotion is driven by tryPromoteToCached once the IC
        // transitions to MONOMORPHIC.
        void handleCallMethodCached(const bytecode::BytecodeProgram::Instruction& instr);

        // MYT-194: GET_FIELD_CACHED / SET_FIELD_CACHED fast paths. Same pattern
        // as handleCallMethodCached — single shape compare against
        // cachedFieldShape, then indexed field access. Shape miss reverts the
        // opcode and re-enters handleGetFieldIC / handleSetFieldIC.
        void handleGetFieldCached(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSetFieldCached(const bytecode::BytecodeProgram::Instruction& instr);

        // MYT-198: superinstruction fast paths. Each fused handler reads the
        // captured LOAD_LOCAL slot from instr.fusedSlot, pushes the receiver,
        // then runs the underlying _CACHED logic. On any stack-shape or shape-
        // miss deopt, tryUnfusePair restores {LOAD_LOCAL, _CACHED} before
        // re-dispatching through the generic IC path.
        void handleLoadLocalCallCached(const bytecode::BytecodeProgram::Instruction& instr);
        void handleLoadLocalGetFieldCached(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        // Shared dispatch body used by the MONO-hit branch of handleCallMethodIC and
        // by handleCallMethodCached. Pops args + receiver, pushes a CallFrame, and
        // sets the instruction pointer to the callee's start offset (minus one, the
        // dispatch loop increments). Assumes funcMeta is non-native with a non-zero
        // startOffset (callers pre-filter).
        void dispatchDirectFromCachedTarget(
            const bytecode::BytecodeProgram::FunctionMetadata* funcMeta,
            size_t startOffset,
            const bytecode::BytecodeProgram* program,
            size_t programIndex,
            bytecode::FunctionNameHandle qualifiedName,
            const std::string& definingClassName,
            value::Value objectValue,
            size_t argCount);

        // MYT-173: promote CALL_METHOD -> CALL_METHOD_CACHED once the IC at the
        // current IP has stabilized to MONOMORPHIC with a bytecode callee. No-op if
        // the site has already demoted once (sticky via cachedDeoptCount).
        void tryPromoteToCached(
            const bytecode::BytecodeProgram::Instruction& instr,
            const vm::jit::ic::MethodICEntry& entry);

        // MYT-173: shape miss on a CACHED site. Rewrites the opcode back to
        // CALL_METHOD, clears cachedShape, bumps cachedDeoptCount (clamped), and
        // re-dispatches through handleCallMethodIC so the IC observes the new shape.
        void deoptAndReprocess(const bytecode::BytecodeProgram::Instruction& instr);

        // MYT-194: promote GET_FIELD / SET_FIELD to their _CACHED variant once
        // the field IC at the current IP has transitioned to MONOMORPHIC.
        // `cachedOp` selects the target opcode (GET or SET); the cleared/set
        // instruction fields are the same. Sticky via cachedFieldDeoptCount.
        void tryPromoteFieldToCached(
            const bytecode::BytecodeProgram::Instruction& instr,
            const runtimeTypes::klass::ClassDefinition* shape,
            size_t fieldIndex,
            bytecode::OpCode cachedOp);

        // MYT-194: shape miss on GET_FIELD_CACHED / SET_FIELD_CACHED. Two
        // concrete helpers rather than one parameterized — matches MYT-173's
        // direct style (revert opcode + re-entry handler are hardcoded).
        void deoptGetFieldAndReprocess(const bytecode::BytecodeProgram::Instruction& instr);
        void deoptSetFieldAndReprocess(const bytecode::BytecodeProgram::Instruction& instr);

        // MYT-198: attempt to fuse the just-promoted CACHED opcode at the
        // current IP with a LOAD_LOCAL at IP-1. On success the LOAD_LOCAL
        // becomes NOP, the current opcode becomes the given fused variant with
        // its fusedSlot set, and all eligibility state is updated. No-op if
        // any guard fails (no prior instr, prior not LOAD_LOCAL, lambda frame,
        // control-flow target, sticky un-fuse). See buildFusionUnsafeTargets.
        void tryFuseWithPriorLoadLocal(bytecode::OpCode fusedOp);

        // MYT-198: restore a fused LOAD_LOCAL_* pair back to {LOAD_LOCAL,
        // underlyingCached}. Called from deopt paths when the fused opcode
        // itself can't dispatch (shape miss, stack shape wrong). Bumps
        // fusedDeoptCount to make the un-fuse decision sticky — a later
        // re-promotion of the CACHED opcode will observe the counter and skip
        // re-fusion. Returns true iff a fusion was actually undone.
        bool tryUnfusePair(bytecode::OpCode underlyingCached);

        ExecutionContext& context;
        vm::jit::ic::InlineCacheTable& icTable;
        ObjectExecutor* objectExecutor = nullptr;
        FunctionExecutor* functionExecutor = nullptr;
    };
}
