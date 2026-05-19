#include "InlineCacheExecutor.hpp"
#include "ObjectExecutor.hpp"
#include "../../../value/SmallArgsBuffer.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace vm::runtime
{
    void InlineCacheExecutor::dispatchDirectFromCachedTarget(const CachedDispatchTarget& target)
    {
        // MYT-196: SBO scratch buffer avoids per-call heap alloc on the MYT-173 hot path.
        value::SmallArgsBuffer args(target.argCount);
        for (size_t i = target.argCount; i > 0; --i)
        {
            args[i - 1] = context.stackManager->pop();
        }
        context.stackManager->pop();

        // Push call frame. MYT-197: 4-byte handle copy replaces std::string copy.
        CallFrame frame;
        frame.returnAddress = context.instructionPointer;
        frame.functionName = target.qualifiedName;
        frame.localBase = context.stackManager->size();
        frame.frameBase = context.stackManager->size();
        // MYT-208: tag-branch `this` ownership. STACK_OBJECT receivers route
        // the raw pointer through thisInstanceRaw (lifetime owned by caller
        // frame's stackObjects); OBJECT keeps the shared_ptr in thisInstance.
        if (value::isStackObject(target.objectValue))
        {
            frame.thisInstanceRaw = value::asObjectInstanceRaw(target.objectValue);
        }
        else
        {
            frame.thisInstance = value::asObject(target.objectValue);
        }
        // MYT-195: pre-resolved at IC populate / CACHED-promote time. Empty
        // when the qualified name carries no "::" prefix — same semantics as
        // the old per-call find/substr that left the field untouched.
        frame.definingClassName = target.definingClassName;
        // MYT-182: carry the callee's program identity on the frame so
        // ControlFlowExecutor restores context.program on return. If the
        // cached entry has no program recorded, fall back to the caller's.
        frame.programIndex = target.program
            ? target.programIndex
            : (context.callStack.empty() ? 0 : context.callStack.back().programIndex);
        context.pushCallFrame(std::move(frame));
        context.stats.functionCalls++;

        // MYT-182: switch context.program to the callee's owning program.
        if (target.program && target.program != context.program)
        {
            context.program = target.program;
        }

        // Push 'this' as first local.
        context.stackManager->push(target.objectValue);

        for (const auto& arg : args)
        {
            context.stackManager->push(arg);
        }

        // Reserve local variable slots beyond 'this' + arguments.
        size_t pushedSlots = 1 + target.argCount;
        if (target.funcMeta->localCount > pushedSlots) {
            for (size_t i = 0; i < target.funcMeta->localCount - pushedSlots; ++i) {
                context.stackManager->push(std::monostate{});
            }
        }

        // Jump to function start (-1 because the dispatch loop increments).
        context.instructionPointer = target.startOffset - 1;
    }

    void InlineCacheExecutor::tryPromoteToCached(
        const bytecode::BytecodeProgram::Instruction& /*instr*/,
        const vm::jit::ic::MethodICEntry& entry)
    {
        using namespace vm::jit::ic;

        const size_t ip = context.instructionPointer;

        // MYT-202: only promote when IP actually holds a generic CALL_METHOD.
        if (context.getMutableInstructionAt(ip).opcode != bytecode::OpCode::CALL_METHOD)
            return;

        // Sticky demote — a site that has already deopted stays on the generic
        // CALL_METHOD path. Prevents flip/unflip ping-pong on unstable sites.
        // MYT-201: read via findCachedState so a never-promoted site doesn't
        // allocate an entry just to read the default 0.
        if (auto* existing = context.findCachedState(ip))
        {
            if (existing->cachedDeoptCount >= 1) return;
        }

        // ValueObject receivers use the MYT-167 temp-materialisation path in
        // jit_call_method; CACHED fast-dispatch is ObjectInstance-only for MVP.
        if (entry.receiverIsValueObject) return;
        // Protocol fast leaves are consumed by the method IC/JIT helper. Do
        // not rewrite them to CALL_METHOD_CACHED, whose hot path dispatches
        // through the generic bytecode mini-interpreter.
        if (entry.protocolFastKind != MethodProtocolFastKind::NONE) return;

        auto* fm = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(entry.funcMetadata);
        if (!fm || fm->isNative || fm->startOffset == 0) return;

        // Only promote once the IC has settled to a single shape. POLY sites
        // are handled by CALL_METHOD_POLY_CACHED (MYT-203).
        MethodInlineCache& cache = icTable.getMethodIC(ip);
        if (cache.state != ICState::MONOMORPHIC) return;

        // MYT-201: writes go into the sparse side table.
        auto& state = context.getOrCreateCachedState(ip);
        state.cachedMethodShape        = entry.shape;
        state.cachedMethodFunc         = fm;
        state.cachedMethodProgram      = entry.program;
        state.cachedMethodProgramIndex = entry.programIndex;
        // MYT-197: intern on the callee's owning program (same program the
        // cached target lives in). Handle is copied 4 bytes at dispatch time.
        state.cachedMethodQualifiedName = entry.program->internFrameName(entry.qualifiedName);
        // MYT-195: snapshot the pre-resolved class prefix alongside qualifiedName.
        state.cachedMethodDefiningClassName = entry.definingClassName;

        auto& mut = context.getMutableInstructionAt(ip);
        mut.opcode = bytecode::OpCode::CALL_METHOD_CACHED;

        // MYT-198: opportunistically fuse with a preceding LOAD_LOCAL.
        tryFuseWithPriorLoadLocal(bytecode::OpCode::LOAD_LOCAL_CALL_CACHED);
    }

    void InlineCacheExecutor::deoptAndReprocess(
        const bytecode::BytecodeProgram::Instruction& /*instr*/)
    {
        const size_t ip = context.instructionPointer;
        auto& mut = context.getMutableInstructionAt(ip);
        // MYT-198: if the CACHED opcode was itself the second half of a fused
        // LOAD_LOCAL_CALL_CACHED pair, we must restore the NOPed LOAD_LOCAL
        // before demoting, otherwise the subsequent CALL_METHOD re-entry would
        // see a NOP predecessor and underflow on the receiver. The fused
        // handler already pushed the receiver onto the stack for this dispatch,
        // so the immediate re-entry below is fine; the un-fuse is for all
        // *future* dispatches of this IP.
        if (mut.opcode == bytecode::OpCode::LOAD_LOCAL_CALL_CACHED)
        {
            tryUnfusePair(bytecode::OpCode::CALL_METHOD_CACHED);
            // Fall through — mut.opcode is now CALL_METHOD_CACHED, demote to
            // CALL_METHOD as usual. Release-active invariant: a future
            // tryUnfusePair regression that left the opcode in another state
            // would silently break the demote, so throw rather than rely on
            // an NDEBUG-stripped assert.
            if (mut.opcode != bytecode::OpCode::CALL_METHOD_CACHED)
            {
                throw errors::RuntimeException(
                    "internal: tryUnfusePair must demote to CALL_METHOD_CACHED");
            }
        }
        mut.opcode = bytecode::OpCode::CALL_METHOD;

        // MYT-201: clear cached fields via the side table. Entry is guaranteed
        // to exist here — a CACHED opcode implies a prior promote.
        auto& state = context.getOrCreateCachedState(ip);
        state.cachedMethodShape = nullptr;
        state.cachedMethodFunc = nullptr;
        state.cachedMethodProgram = nullptr;
        state.cachedMethodProgramIndex = 0;
        // MYT-197: integer sentinel replaces std::string::clear(). A re-promote
        // that happens to observe a stale handle value would index into the
        // interner and return an unrelated qualified name; INVALID_FN_HANDLE
        // prevents that.
        state.cachedMethodQualifiedName = bytecode::INVALID_FN_HANDLE;
        // MYT-195: clear the pre-resolved class prefix in lockstep with
        // qualifiedName so a subsequent re-promotion doesn't pick up a stale
        // class name from the prior shape.
        state.cachedMethodDefiningClassName.clear();
        if (state.cachedDeoptCount < 255) ++state.cachedDeoptCount;
        // Re-enter the generic IC path, which will observe the new shape and
        // (unless already MEGA) transition MONO -> POLY.
        handleCallMethodIC(mut);
    }

    void InlineCacheExecutor::handleCallMethodCached(
        const bytecode::BytecodeProgram::Instruction& instr,
        const bytecode::BytecodeProgram::CachedInstructionState& state)
    {
        if (objectExecutor && objectExecutor->tryDispatchSpecializedCollectionCall(instr))
        {
            return;
        }

        if (instr.numOperands() < 2 || !state.cachedMethodFunc || !state.cachedMethodShape)
        {
            deoptAndReprocess(instr);
            return;
        }

        size_t argCount = instr.inlineOperands[1];
        if (context.stackManager->size() <= argCount)
        {
            deoptAndReprocess(instr);
            return;
        }

        value::Value objectValue = context.stackManager->peek(argCount);
        // MYT-208: accept STACK_OBJECT receivers — the cached shape is keyed
        // by ClassDefinition*, identical for OBJECT and STACK_OBJECT.
        if (!value::isAnyObject(objectValue))
        {
            deoptAndReprocess(instr);
            return;
        }

        auto* instance = value::asObjectInstanceRaw(objectValue);
        auto* shape = instance->getClassDefinitionRaw();
        if (shape != state.cachedMethodShape)
        {
            deoptAndReprocess(instr);
            return;
        }

        // Hot path: shape matched — dispatch directly from the embedded target.
        // No icTable.getMethodIC probe, no per-entry scan.
        dispatchDirectFromCachedTarget({
            state.cachedMethodFunc,
            state.cachedMethodFunc->startOffset,
            state.cachedMethodProgram,
            state.cachedMethodProgramIndex,
            state.cachedMethodQualifiedName,
            state.cachedMethodDefiningClassName,
            objectValue,
            argCount
        });
    }
}
