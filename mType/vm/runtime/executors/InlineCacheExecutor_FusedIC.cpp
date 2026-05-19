#include "InlineCacheExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../../../constants/SecurityConstants.hpp"
#include <cassert>

namespace vm::runtime
{
    void InlineCacheExecutor::tryFuseWithPriorLoadLocal(bytecode::OpCode fusedOp)
    {
        const size_t ip = context.instructionPointer;
        if (ip == 0) return;

        // Prior must be LOAD_LOCAL within the same program. loadedPrograms
        // routing isn't relevant — IP and program are already coherent for the
        // currently-executing frame.
        const auto& prev = context.program->getInstruction(ip - 1);
        if (prev.opcode != bytecode::OpCode::LOAD_LOCAL) return;
        if (prev.numOperands() == 0) return;

        // Current IP must not be reachable via any control-flow path other than
        // falling through from IP-1. Otherwise a jump landing directly on the
        // fused op would execute LOAD_LOCAL(fusedSlot) on a stack that the
        // jumping caller didn't set up for it.
        if (context.program->isFusionUnsafeTarget(ip)) return;

        // MYT-201: fusion reads/writes live on the side-table entry. Current
        // IP's entry is guaranteed to exist (caller just promoted to CACHED).
        auto& state = context.getOrCreateCachedState(ip);

        // Sticky un-fuse — a site that already had its fusion rolled back
        // should not be re-fused. Mirrors cachedDeoptCount's sticky semantics.
        if (state.fusedDeoptCount >= 1) return;

        // Lambda / shared-frame guard. LOAD_LOCAL inside a lambda frame has
        // captured-variable semantics that the fused executor doesn't replicate
        // (it reads from frameBase + slot directly). Skipping here is safer
        // than duplicating VariableExecutor::handleLoadLocal's capture logic
        // in every fused op; lambda-body fusion is follow-up.
        if (!context.callStack.empty())
        {
            const auto& frame = context.callStack.back();
            if (frame.originatingLambda || frame.sharedFrame) return;
        }

        // MYT-173 precedent: an in-place opcode rewrite (prev→NOP, current→
        // fused) keeps the instruction vector length stable, so no jump
        // offsets need fixing up. fusedSlot captures what used to be the
        // LOAD_LOCAL's operand[0].
        uint64_t slot = prev.inlineOperands[0];
        // fusedSlot is uint32_t — assert before truncation. Slot indices are
        // capped by constants::security::MAX_LOCAL_STACK_PER_FRAME at every
        // LOAD_LOCAL / STORE_LOCAL entry, so this should never fire in a
        // well-formed program, but an oversized operand would otherwise
        // silently alias a different slot at the fused site.
        assert(slot <= UINT32_MAX &&
               "LOAD_LOCAL fusion: slot index exceeds fusedSlot width");
        auto& prevMut = context.getMutableInstructionAt(ip - 1);
        prevMut.opcode = bytecode::OpCode::NOP;
        prevMut.clearOperands();

        state.fusedSlot = static_cast<uint32_t>(slot);
        auto& mut = context.getMutableInstructionAt(ip);
        mut.opcode = fusedOp;
    }

    void InlineCacheExecutor::handleLoadLocalCallCached(
        const bytecode::BytecodeProgram::Instruction& instr,
        const bytecode::BytecodeProgram::CachedInstructionState& state)
    {
        // Fast replay of the NOPed LOAD_LOCAL: the fusion trigger guaranteed
        // a non-lambda, non-shared frame, so the simple localBase + slot read
        // is sufficient — we don't need to replicate handleLoadLocal's
        // capture-chain logic. SECURITY: same MAX_LOCAL_STACK_PER_FRAME cap
        // as handleLoadLocal, on fusedSlot.
        size_t slot = static_cast<size_t>(state.fusedSlot);
        if (slot >= constants::security::MAX_LOCAL_STACK_PER_FRAME)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "LOAD_LOCAL_CALL_CACHED slot index " + std::to_string(slot) +
                " exceeds per-frame limit");
        }

        size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
        size_t stackPos = frameBase + slot;
        if (stackPos >= context.stackManager->size())
        {
            // Stack shape doesn't match the fusion expectation — un-fuse and
            // re-dispatch through the generic IC path. tryUnfusePair bumps
            // fusedDeoptCount so we don't re-fuse.
            tryUnfusePair(bytecode::OpCode::CALL_METHOD_CACHED);
            // Receiver isn't on stack; replay through handleCallMethodIC which
            // will fail with the correct error at the generic site.
            handleCallMethodIC(
                context.getMutableInstructionAt(context.instructionPointer));
            return;
        }
        context.stackManager->push((*context.stackManager)[stackPos]);

        // Receiver now on stack; delegate to the standard CACHED handler.
        // Same state entry — no second side-table lookup needed.
        handleCallMethodCached(instr, state);
    }

    void InlineCacheExecutor::handleLoadLocalCallPolyCached(
        const bytecode::BytecodeProgram::Instruction& instr,
        const bytecode::BytecodeProgram::CachedInstructionState& state)
    {
        // MYT-203: same fast-replay pattern as handleLoadLocalCallCached.
        // Fusion-time guards (non-lambda, non-shared frame) make the simple
        // localBase + slot read sufficient.
        size_t slot = static_cast<size_t>(state.fusedSlot);
        if (slot >= constants::security::MAX_LOCAL_STACK_PER_FRAME)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "LOAD_LOCAL_CALL_POLY_CACHED slot index " + std::to_string(slot) +
                " exceeds per-frame limit");
        }

        size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
        size_t stackPos = frameBase + slot;
        if (stackPos >= context.stackManager->size())
        {
            tryUnfusePair(bytecode::OpCode::CALL_METHOD_POLY_CACHED);
            handleCallMethodIC(
                context.getMutableInstructionAt(context.instructionPointer));
            return;
        }
        context.stackManager->push((*context.stackManager)[stackPos]);

        handleCallMethodPolyCached(instr, state);
    }

    void InlineCacheExecutor::handleLoadLocalGetFieldCached(
        const bytecode::BytecodeProgram::Instruction& instr,
        const bytecode::BytecodeProgram::CachedInstructionState& state)
    {
        size_t slot = static_cast<size_t>(state.fusedSlot);
        if (slot >= constants::security::MAX_LOCAL_STACK_PER_FRAME)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "LOAD_LOCAL_GET_FIELD_CACHED slot index " + std::to_string(slot) +
                " exceeds per-frame limit");
        }

        size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
        size_t stackPos = frameBase + slot;
        if (stackPos >= context.stackManager->size())
        {
            tryUnfusePair(bytecode::OpCode::GET_FIELD_CACHED);
            handleGetFieldIC(
                context.getMutableInstructionAt(context.instructionPointer));
            return;
        }
        context.stackManager->push((*context.stackManager)[stackPos]);

        handleGetFieldCached(instr, state);
    }

    bool InlineCacheExecutor::tryUnfusePair(bytecode::OpCode underlyingCached)
    {
        const size_t ip = context.instructionPointer;
        if (ip == 0) return false;

        auto& mut = context.getMutableInstructionAt(ip);
        if (mut.opcode != bytecode::OpCode::LOAD_LOCAL_CALL_CACHED &&
            mut.opcode != bytecode::OpCode::LOAD_LOCAL_CALL_POLY_CACHED &&
            mut.opcode != bytecode::OpCode::LOAD_LOCAL_GET_FIELD_CACHED)
        {
            return false;
        }

        // MYT-201: fused state lives in the side table. Entry is guaranteed
        // to exist (this IP was fused → promoted → an entry was created).
        auto& state = context.getOrCreateCachedState(ip);

        // Restore LOAD_LOCAL at IP-1 from the captured fusedSlot.
        auto& prevMut = context.getMutableInstructionAt(ip - 1);
        prevMut.opcode = bytecode::OpCode::LOAD_LOCAL;
        prevMut.setSingleOperand(static_cast<uint64_t>(state.fusedSlot));

        // Demote current back to the underlying CACHED opcode and make the
        // un-fuse sticky so the next promotion here won't re-fuse.
        mut.opcode = underlyingCached;
        if (state.fusedDeoptCount < 255) ++state.fusedDeoptCount;
        return true;
    }
}
