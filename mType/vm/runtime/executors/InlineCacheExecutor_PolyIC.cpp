#include "InlineCacheExecutor.hpp"
#include "ObjectExecutor.hpp"
#include "../../../errors/RuntimeException.hpp"
#include <algorithm>
#include <tuple>

namespace vm::runtime
{
    void InlineCacheExecutor::tryPromoteToPolyCached(
        const bytecode::BytecodeProgram::Instruction& /*instr*/,
        const vm::jit::ic::MethodICEntry& /*triggeringEntry*/)
    {
        using namespace vm::jit::ic;

        const size_t ip = context.instructionPointer;
        auto& mut = context.getMutableInstructionAt(ip);

        // Predecessor must be generic CALL_METHOD (the IC just transitioned
        // MONO→POLY via deopt + re-enter, or was POLY from the start) OR an
        // already-POLY_CACHED site re-snapshotting on a 3rd / 4th shape. Reject
        // CALL_METHOD_CACHED — that path's deopt would have demoted it to
        // CALL_METHOD before we got here.
        if (mut.opcode != bytecode::OpCode::CALL_METHOD &&
            mut.opcode != bytecode::OpCode::CALL_METHOD_POLY_CACHED)
            return;

        // Tier-specific sticky gate — independent of MYT-173's cachedDeoptCount.
        // A site that deopted at the MONO tier can still reach POLY_CACHED on
        // its next stable phase.
        if (auto* existing = context.findCachedState(ip))
        {
            if (existing->polyCachedDeoptCount >= 1) return;
        }

        MethodInlineCache& cache = icTable.getMethodIC(ip);
        if (cache.state != ICState::POLYMORPHIC) return;

        // All-or-nothing per-entry guards. A site with ANY ValueObject /
        // native / zero-startOffset entry sinks the whole site (we can't
        // partially specialize the linear scan). Acceptable per MYT-173
        // precedent; ValueObject method dispatch is its own track.
        for (uint8_t i = 0; i < cache.entryCount; ++i)
        {
            const auto& e = cache.entries[i];
            if (e.receiverIsValueObject) return;
            if (e.protocolFastKind != MethodProtocolFastKind::NONE) return;
            auto* fm = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(e.funcMetadata);
            if (!fm || fm->isNative || fm->startOffset == 0) return;
        }

        // INVARIANT: MethodInlineCache::addEntry is append-only. If a future
        // change introduces LRU reordering, the snapshot must be revisited
        // (the per-IP cached state would need to track entry identity, not
        // index, to remain consistent across re-snapshots).
        auto& state = context.getOrCreateCachedState(ip);
        // Defensive clamp: the side-table arrays are size 4. A future IC
        // change that lets entryCount exceed 4 must not silently OOB the
        // poly* arrays here or in handleCallMethodPolyCached's linear scan.
        constexpr uint8_t kMaxPolyEntries =
            static_cast<uint8_t>(std::tuple_size<decltype(state.polyShapes)>::value);
        const uint8_t snapshotCount =
            std::min<uint8_t>(cache.entryCount, kMaxPolyEntries);
        for (uint8_t i = 0; i < snapshotCount; ++i)
        {
            const auto& e = cache.entries[i];
            state.polyShapes[i]             = e.shape;
            state.polyFuncs[i]              = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(e.funcMetadata);
            state.polyPrograms[i]           = e.program;
            state.polyProgramIndices[i]     = e.programIndex;
            state.polyQualifiedNames[i]     = e.program->internFrameName(e.qualifiedName);
            state.polyDefiningClassNames[i] = e.definingClassName;
        }
        state.polyEntryCount = snapshotCount;
        mut.opcode = bytecode::OpCode::CALL_METHOD_POLY_CACHED;

        // MYT-198 + MYT-203: opportunistically fuse with a preceding LOAD_LOCAL.
        // Same eligibility gates as the MONO variant; reuses fusedSlot +
        // fusedDeoptCount on the side table.
        tryFuseWithPriorLoadLocal(bytecode::OpCode::LOAD_LOCAL_CALL_POLY_CACHED);
    }

    void InlineCacheExecutor::deoptPolyAndReprocess(
        const bytecode::BytecodeProgram::Instruction& /*instr*/)
    {
        const size_t ip = context.instructionPointer;
        auto& mut = context.getMutableInstructionAt(ip);
        // MYT-198 / MYT-203: if the POLY_CACHED opcode was the second half of
        // a fused LOAD_LOCAL_CALL_POLY_CACHED pair, restore the NOPed LOAD_LOCAL
        // before demoting. Same pattern as deoptAndReprocess.
        if (mut.opcode == bytecode::OpCode::LOAD_LOCAL_CALL_POLY_CACHED)
        {
            tryUnfusePair(bytecode::OpCode::CALL_METHOD_POLY_CACHED);
            // Release-active invariant — see deoptAndReprocess for rationale.
            if (mut.opcode != bytecode::OpCode::CALL_METHOD_POLY_CACHED)
            {
                throw errors::RuntimeException(
                    "internal: tryUnfusePair must demote to CALL_METHOD_POLY_CACHED");
            }
        }
        mut.opcode = bytecode::OpCode::CALL_METHOD;

        // MYT-201: the per-IP cached state is guaranteed to exist (a POLY_CACHED
        // opcode implies a prior promote). Validate the invariant via
        // findCachedState first so a violation — e.g. side table cleared while
        // a POLY_CACHED opcode was still live — fails loudly instead of
        // silently allocating a fresh entry. The subsequent getOrCreateCachedState
        // then resolves to the existing entry (the "create" branch is dead
        // after the find succeeds). Zero polyEntryCount so any subsequent
        // (mis)dispatch through handleCallMethodPolyCached sees an empty
        // snapshot — the arrays themselves are left stale, since the demoted
        // opcode never reads them and the sticky polyCachedDeoptCount
        // guarantees no re-promote.
        if (!context.findCachedState(ip))
        {
            throw errors::RuntimeException(
                "internal: deoptPolyAndReprocess invoked without a cached state");
        }
        auto& state = context.getOrCreateCachedState(ip);
        state.polyEntryCount = 0;
        if (state.polyCachedDeoptCount < 255) ++state.polyCachedDeoptCount;

        // Re-enter the generic IC path so the receiver's shape gets observed
        // and the call is actually dispatched.
        handleCallMethodIC(mut);
    }

    void InlineCacheExecutor::handleCallMethodPolyCached(
        const bytecode::BytecodeProgram::Instruction& instr,
        const bytecode::BytecodeProgram::CachedInstructionState& state)
    {
        if (objectExecutor && objectExecutor->tryDispatchSpecializedCollectionCall(instr))
        {
            return;
        }

        if (instr.numOperands() < 2 || state.polyEntryCount == 0)
        {
            deoptPolyAndReprocess(instr);
            return;
        }

        size_t argCount = instr.inlineOperands[1];
        if (context.stackManager->size() <= argCount)
        {
            deoptPolyAndReprocess(instr);
            return;
        }

        value::Value objectValue = context.stackManager->peek(argCount);
        // MYT-208: accept STACK_OBJECT receivers (shape key is ClassDefinition*).
        if (!value::isAnyObject(objectValue))
        {
            deoptPolyAndReprocess(instr);
            return;
        }

        auto* instance = value::asObjectInstanceRaw(objectValue);
        auto* shape = instance->getClassDefinitionRaw();

        // Linear scan over the inline POLY entries. The compiler unrolls /
        // branch-predicts a 4-iter loop comfortably; hand-unrolling buys
        // nothing and hurts readability. Hot-path total: 1-4 pointer compares
        // + 1 direct dispatch (no icTable hashmap probe, no per-entry IC scan).
        // Clamp to array size as belt-and-braces against a corrupted
        // polyEntryCount even though tryPromoteToPolyCached caps it at write.
        const uint8_t scanCount = std::min<uint8_t>(state.polyEntryCount,
            static_cast<uint8_t>(std::tuple_size<decltype(state.polyShapes)>::value));
        for (uint8_t i = 0; i < scanCount; ++i)
        {
            if (state.polyShapes[i] == shape)
            {
                const auto* funcMeta = state.polyFuncs[i];
                dispatchDirectFromCachedTarget({
                    funcMeta,
                    funcMeta->startOffset,
                    state.polyPrograms[i],
                    state.polyProgramIndices[i],
                    state.polyQualifiedNames[i],
                    state.polyDefiningClassNames[i],
                    objectValue,
                    argCount
                });
                return;
            }
        }

        // Linear-scan miss. Could be either a 3rd / 4th shape (POLY-stable,
        // re-snapshot on slow-path return) or an overflow shape (MEGA, demote
        // in the slow-path's MEGA-detect block). Either way, delegate to
        // handleCallMethodIC — do NOT bump sticky here, that would block
        // legitimate POLY growth.
        handleCallMethodIC(
            context.getMutableInstructionAt(context.instructionPointer));
    }
}
