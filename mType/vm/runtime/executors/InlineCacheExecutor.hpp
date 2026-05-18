#pragma once
#include "../context/ExecutionContext.hpp"
#include <cstddef>
#include "../../jit/ic/InlineCacheTable.hpp"
#include "../../jit/ic/InlineCacheTypes.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/NullPointerException.hpp"
#include "../../../errors/FieldNotFoundException.hpp"
#include "../../../value/ValueShim.hpp"
#include "../utils/NullCheckUtils.hpp"
#include "../utils/ErrorLocationHelper.hpp"

namespace vm::runtime
{
    class ObjectExecutor;
    class FunctionExecutor;
    class VirtualMachine;

    class InlineCacheExecutor
    {
    public:
        InlineCacheExecutor(ExecutionContext& ctx,
                            VirtualMachine* vmPtr,
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

        // MYT-173: CALL_METHOD_CACHED fast path. Reads the cached target from
        // the per-IP side table (MYT-201), does a single shape compare against
        // cachedMethodShape, and dispatches directly (no IC hashmap probe, no
        // per-entry linear scan). On shape miss, rewrites the opcode back to
        // CALL_METHOD and re-enters the generic IC path. Promotion is driven
        // by tryPromoteToCached once the IC transitions to MONOMORPHIC.
        // MYT-201: `state` is the per-IP cached state; dispatch loop looks it
        // up once and hands the reference down.
        void handleCallMethodCached(
            const bytecode::BytecodeProgram::Instruction& instr,
            const bytecode::BytecodeProgram::CachedInstructionState& state);

        // MYT-203: CALL_METHOD_POLY_CACHED fast path. Linear-scans up to 4
        // embedded shapes from the side table; on hit, dispatches directly
        // (no icTable hashmap probe, no per-entry scan over the IC table).
        // On miss after all snapshotted entries fail, deopts via
        // deoptPolyAndReprocess. Promotion is driven by tryPromoteToPolyCached
        // once the IC transitions to POLYMORPHIC. Independent sticky counter
        // (polyCachedDeoptCount) so a MONO-tier deopt does not block POLY
        // promotion.
        void handleCallMethodPolyCached(
            const bytecode::BytecodeProgram::Instruction& instr,
            const bytecode::BytecodeProgram::CachedInstructionState& state);

        // MYT-194 + MYT-319: GET_FIELD_CACHED / SET_FIELD_CACHED fast paths
        // inlined here for dispatch inlining. Single shape compare against
        // cachedFieldShape, then indexed field access. Shape miss reverts the
        // opcode and re-enters handleGetFieldIC / handleSetFieldIC via the
        // out-of-line deopt helpers.
        inline void handleGetFieldCached(
            const bytecode::BytecodeProgram::Instruction& instr,
            const bytecode::BytecodeProgram::CachedInstructionState& state)
        {
            if (instr.numOperands() == 0 || !state.cachedFieldShape
                || state.cachedFieldIndex == SIZE_MAX)
            {
                deoptGetFieldAndReprocess(instr);
                return;
            }
            if (context.stackManager->size() < 1)
            {
                deoptGetFieldAndReprocess(instr);
                return;
            }

            // Peek so that a deopt leaves the stack untouched.
            value::Value objectValue = context.stackManager->peek(0);

            // Null check (respect the compiler's NONNULL_RECEIVER flag).
            if (!(instr.flags & bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER))
            {
                if (utils::isNullValue(objectValue))
                {
                    const std::string& fieldName =
                        context.program->getConstantPool().getString(instr.inlineOperands[0]);
                    utils::ErrorLocationHelper::throwError<errors::NullPointerException>(
                        context,
                        "Cannot access field '" + fieldName + "' on null object");
                }
            }

            // MYT-208: accept STACK_OBJECT (shape key is ClassDefinition*).
            if (!value::isAnyObject(objectValue))
            {
                deoptGetFieldAndReprocess(instr);
                return;
            }

            auto* instance = value::asObjectInstanceRaw(objectValue);
            auto* shape = instance->getClassDefinitionRaw();
            if (shape != state.cachedFieldShape)
            {
                deoptGetFieldAndReprocess(instr);
                return;
            }

            // Hot path: shape matched — indexed read, no IC probe.
            if (!instance->hasFieldVector())
            {
                instance->ensureFieldVector();
            }
            value::Value fieldValue = instance->getFieldByIndex(state.cachedFieldIndex);
            context.stackManager->pop();
            context.stackManager->push(fieldValue);
        }

        inline void handleSetFieldCached(
            const bytecode::BytecodeProgram::Instruction& instr,
            const bytecode::BytecodeProgram::CachedInstructionState& state)
        {
            if (instr.numOperands() == 0 || !state.cachedFieldShape
                || state.cachedFieldIndex == SIZE_MAX)
            {
                deoptSetFieldAndReprocess(instr);
                return;
            }
            if (context.stackManager->size() < 2)
            {
                deoptSetFieldAndReprocess(instr);
                return;
            }

            // Stack: [..., object, newValue]. peek(0)=newValue, peek(1)=object.
            value::Value newValue = context.stackManager->peek(0);
            value::Value objectValue = context.stackManager->peek(1);

            if (!(instr.flags & bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER))
            {
                if (utils::isNullValue(objectValue))
                {
                    const std::string& fieldName =
                        context.program->getConstantPool().getString(instr.inlineOperands[0]);
                    utils::ErrorLocationHelper::throwError<errors::NullPointerException>(
                        context,
                        "Cannot set field '" + fieldName + "' on null object");
                }
            }

            // MYT-208: accept STACK_OBJECT (shape key is ClassDefinition*).
            if (!value::isAnyObject(objectValue))
            {
                deoptSetFieldAndReprocess(instr);
                return;
            }

            auto* instance = value::asObjectInstanceRaw(objectValue);
            auto* shape = instance->getClassDefinitionRaw();
            if (shape != state.cachedFieldShape)
            {
                deoptSetFieldAndReprocess(instr);
                return;
            }

            if (!instance->hasFieldVector())
            {
                instance->ensureFieldVector();
            }
            instance->setFieldByIndex(state.cachedFieldIndex, newValue);
            // Match handleSetFieldIC: consume both operands, push newValue for
            // assignment chaining.
            context.stackManager->pop();
            context.stackManager->pop();
            context.stackManager->push(newValue);
        }

        // MYT-198: superinstruction fast paths. Each fused handler reads the
        // captured LOAD_LOCAL slot from state.fusedSlot, pushes the receiver,
        // then runs the underlying _CACHED logic. On any stack-shape or shape-
        // miss deopt, tryUnfusePair restores {LOAD_LOCAL, _CACHED} before
        // re-dispatching through the generic IC path.
        void handleLoadLocalCallCached(
            const bytecode::BytecodeProgram::Instruction& instr,
            const bytecode::BytecodeProgram::CachedInstructionState& state);

        // MYT-203: fused LOAD_LOCAL + CALL_METHOD_POLY_CACHED. Replays the
        // NOPed LOAD_LOCAL via state.fusedSlot, then defers to
        // handleCallMethodPolyCached. Stack-shape failure tryUnfusePair's
        // back to {LOAD_LOCAL, CALL_METHOD_POLY_CACHED} and re-dispatches.
        void handleLoadLocalCallPolyCached(
            const bytecode::BytecodeProgram::Instruction& instr,
            const bytecode::BytecodeProgram::CachedInstructionState& state);

        void handleLoadLocalGetFieldCached(
            const bytecode::BytecodeProgram::Instruction& instr,
            const bytecode::BytecodeProgram::CachedInstructionState& state);

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

        // MYT-203: promote CALL_METHOD or already-POLY_CACHED to
        // CALL_METHOD_POLY_CACHED once the IC at the current IP is
        // POLYMORPHIC and every entry is bytecode-callable (non-native,
        // non-ValueObject, non-zero startOffset). Idempotent: a 3rd or 4th
        // shape arrival re-snapshots all entries[0..entryCount-1]. Sticky
        // via polyCachedDeoptCount (independent of MYT-173's
        // cachedDeoptCount).
        void tryPromoteToPolyCached(
            const bytecode::BytecodeProgram::Instruction& instr,
            const vm::jit::ic::MethodICEntry& entry);

        // MYT-203: shape miss on a POLY_CACHED site (linear-scan exhausted)
        // OR POLY→MEGA transition (5th shape). Rewrites the opcode back to
        // CALL_METHOD, zeroes polyEntryCount (entries themselves left
        // stale), bumps polyCachedDeoptCount, and re-dispatches through
        // handleCallMethodIC. Mirrors deoptAndReprocess but for the POLY
        // tier with a tier-specific sticky counter.
        void deoptPolyAndReprocess(const bytecode::BytecodeProgram::Instruction& instr);

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
        VirtualMachine* vm;
        vm::jit::ic::InlineCacheTable& icTable;
        ObjectExecutor* objectExecutor = nullptr;
        FunctionExecutor* functionExecutor = nullptr;
    };
}
