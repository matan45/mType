#include "VirtualMachine.hpp"
#include <cassert>
#include <cstdint>
#include <exception>
#include "../../value/SmallArgsBuffer.hpp"
#include "../jit/JitCodeCache.hpp"
#include "../jit/JitContext.hpp"
#include "../jit/ScopedJitNativeDepth.hpp"
#include "../jit/guards/DeoptimizationHandler.hpp"
#include "../jit/ic/InlineCacheTable.hpp"
#include "../jit/ic/TypeFeedbackCollector.hpp"
#include "executors/FunctionExecutor.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../bytecode/OpCode.hpp"

namespace vm::runtime
{
    namespace
    {
        // MYT-268: shared deopt-detection idiom for executeCallWithJit /
        // executeCallFastWithJit. jit_await stashes OSRDeoptException on
        // ctx->pendingException instead of throwing (the throw form crashed
        // silently on Windows x64 — no PE unwind data registered for the
        // asmjit frame). std::exception_ptr can't be peeked without
        // rethrowing; this helper rethrows once to inspect the type, and
        // either reports "deopt sentinel" or rethrows real errors so the
        // VM loop sees them exactly as the previous direct rethrow did.
        bool isStashedOSRDeopt(std::exception_ptr ep)
        {
            try
            {
                std::rethrow_exception(ep);
            }
            catch (const jit::OSRDeoptException&)
            {
                return true;
            }
            catch (...)
            {
                std::rethrow_exception(ep);
            }
            return false; // unreachable
        }
    }

    void VirtualMachine::trySpecializeArithmetic(
        const bytecode::BytecodeProgram::Instruction& instr,
        bytecode::OpCode intOpcode, bytecode::OpCode floatOpcode)
    {
        if (!icEnabled || !typeFeedbackCollector || stackManager->size() < 2)
            return;
        const auto* activeProgram = executionCtx ? executionCtx->program : program;
        if (!activeProgram) return;
        const auto programId = activeProgram->getProgramId();

        typeFeedbackCollector->recordBinaryOp(
            programId, instructionPointer,
            stackManager->peek(1), stackManager->peek(0));

        if (!typeFeedbackCollector->shouldSpecialize(programId, instructionPointer))
            return;

        auto [lt, rt] = typeFeedbackCollector->getDominantTypes(
            programId, instructionPointer);
        if (lt == jit::ic::ObservedType::INT && rt == jit::ic::ObservedType::INT)
        {
            const_cast<bytecode::BytecodeProgram::Instruction&>(instr).opcode = intOpcode;
            inlineCacheTable->getTypeFeedback(
                programId, instructionPointer).specialized = true;

            // MYT-198: try to fuse ADD_INT with a preceding PUSH_INT into
            // ADD_INT_CONST. Only ADD_INT is covered by the fused opcode set
            // for MVP (the same pattern would extend to SUB/MUL/DIV trivially).
            if (intOpcode == bytecode::OpCode::ADD_INT)
            {
                tryFuseAddIntConst();
            }
        }
        else if (lt == jit::ic::ObservedType::FLOAT && rt == jit::ic::ObservedType::FLOAT)
        {
            const_cast<bytecode::BytecodeProgram::Instruction&>(instr).opcode = floatOpcode;
            inlineCacheTable->getTypeFeedback(
                programId, instructionPointer).specialized = true;
        }
    }

    void VirtualMachine::trySpecializeBitwise(
        const bytecode::BytecodeProgram::Instruction& instr,
        bytecode::OpCode intOpcode)
    {
        if (!icEnabled || !typeFeedbackCollector || stackManager->size() < 2)
            return;
        const auto* activeProgram = executionCtx ? executionCtx->program : program;
        if (!activeProgram) return;
        const auto programId = activeProgram->getProgramId();

        // Sticky-demote gate. The TypeFeedback.specialized flag already
        // blocks re-entry via shouldSpecialize, but if any future demote
        // path clears it (mirroring MYT-173 CALL_METHOD_CACHED deopts), the
        // cachedDeoptCount check prevents oscillation on a site that has
        // already un-specialized once.
        if (auto* existing = activeProgram->findCachedState(instructionPointer))
        {
            if (existing->cachedDeoptCount >= 1) return;
        }

        typeFeedbackCollector->recordBinaryOp(
            programId, instructionPointer,
            stackManager->peek(1), stackManager->peek(0));

        if (!typeFeedbackCollector->shouldSpecialize(programId, instructionPointer))
            return;

        auto [lt, rt] = typeFeedbackCollector->getDominantTypes(
            programId, instructionPointer);
        if (lt == jit::ic::ObservedType::INT && rt == jit::ic::ObservedType::INT)
        {
            const_cast<bytecode::BytecodeProgram::Instruction&>(instr).opcode = intOpcode;
            inlineCacheTable->getTypeFeedback(
                programId, instructionPointer).specialized = true;
        }
    }

    void VirtualMachine::trySpecializeBitwiseUnary(
        const bytecode::BytecodeProgram::Instruction& instr,
        bytecode::OpCode intOpcode)
    {
        if (!icEnabled || !typeFeedbackCollector || stackManager->size() < 1)
            return;
        const auto* activeProgram = executionCtx ? executionCtx->program : program;
        if (!activeProgram) return;
        const auto programId = activeProgram->getProgramId();

        if (auto* existing = activeProgram->findCachedState(instructionPointer))
        {
            if (existing->cachedDeoptCount >= 1) return;
        }

        // Reuse recordBinaryOp with the same operand twice — the feedback
        // collector only inspects tag, and duplicating avoids adding a
        // separate unary path for a one-operand promotion.
        const value::Value& tos = stackManager->peek(0);
        typeFeedbackCollector->recordBinaryOp(
            programId, instructionPointer, tos, tos);

        if (!typeFeedbackCollector->shouldSpecialize(programId, instructionPointer))
            return;

        auto [lt, rt] = typeFeedbackCollector->getDominantTypes(
            programId, instructionPointer);
        if (lt == jit::ic::ObservedType::INT && rt == jit::ic::ObservedType::INT)
        {
            const_cast<bytecode::BytecodeProgram::Instruction&>(instr).opcode = intOpcode;
            inlineCacheTable->getTypeFeedback(
                programId, instructionPointer).specialized = true;
        }
    }

    void VirtualMachine::tryFuseAddIntConst()
    {
        auto* activeProgram = executionCtx ? executionCtx->program : program;
        if (!activeProgram) return;

        const size_t ip = instructionPointer;
        if (ip == 0) return;

        // Prior must be PUSH_INT; its operand[0] is a constant-pool index
        // (see StackOperationsExecutor::handlePushInt). ADD_INT_CONST stores
        // that same index in fusedSlot and reads the literal from the pool
        // at dispatch time — no eager resolution.
        const auto& prev = activeProgram->getInstruction(ip - 1);
        if (prev.opcode != bytecode::OpCode::PUSH_INT) return;
        if (prev.numOperands() == 0) return;

        // Same fusion-safety gates as the LOAD_LOCAL fusions: no control-flow
        // target may land directly on the fused op, and sticky un-fuse blocks
        // re-fusion.
        if (activeProgram->isFusionUnsafeTarget(ip)) return;

        // MYT-201: fused state lives in the per-IP side table. Sticky un-fuse
        // gate reads via findCachedState so a never-fused site doesn't
        // allocate an entry just to check the default 0.
        if (auto* existing = activeProgram->findCachedState(ip))
        {
            if (existing->fusedDeoptCount >= 1) return;
        }

        uint64_t constIdx = prev.inlineOperands[0];
        // fusedSlot is uint32_t — assert before truncation so an oversized
        // constant-pool index surfaces as a clean failure rather than silent
        // data loss. Constant pools are bounded by the compiler, but the
        // cast is attacker-reachable via a malformed .mtc operand.
        assert(constIdx <= UINT32_MAX &&
               "ADD_INT_CONST fusion: PUSH_INT operand exceeds fusedSlot width");
        auto& prevMut = const_cast<bytecode::BytecodeProgram*>(activeProgram)
                            ->getMutableInstruction(ip - 1);
        prevMut.opcode = bytecode::OpCode::NOP;
        prevMut.clearOperands();

        auto& state = activeProgram->getOrCreateCachedState(ip);
        state.fusedSlot = static_cast<uint32_t>(constIdx);

        auto& mut = const_cast<bytecode::BytecodeProgram*>(activeProgram)
                        ->getMutableInstruction(ip);
        mut.opcode = bytecode::OpCode::ADD_INT_CONST;
    }

    void VirtualMachine::executeCallWithJit(
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (jitEnabled && jitCodeCache && jitNativeDepth < MAX_JIT_NATIVE_DEPTH
            && !inJitFallbackInterpreter)
        {
            const auto* activeProgram = executionCtx ? executionCtx->program : program;
            std::string funcName = activeProgram->getConstantPool().getString(instr.inlineOperands[0]);
            auto jitCode = jitCodeCache->lookup(
                activeProgram->getProgramId(), funcName);
            if (jitCode)
            {
                size_t argCount = instr.inlineOperands[1];

                // MYT-196: small-buffer-optimized args for JIT entry.
                value::SmallArgsBuffer args(argCount);
                for (size_t i = argCount; i > 0; --i)
                    args[i - 1] = stackManager->pop();

                jit::JitContext jitCtx{};
                jitCtx.args = args.data();
                jitCtx.argCount = args.size();
                jitCtx.hasReturnValue = false;
                jitCtx.program = activeProgram;
                jitCtx.stackManager = stackManager.get();
                jitCtx.environment = environment.get();
                jitCtx.vm = this;
                jitCtx.jitCodeCache = jitCodeCache.get();
                jitCtx.icTable = inlineCacheTable.get();

                size_t sepPos = funcName.find("::");
                if (sepPos != std::string::npos)
                    jitCtx.callingClassName = funcName.substr(0, sepPos);

                {
                    jit::ScopedJitNativeDepth nativeFrame(*this);
                    jitCode(&jitCtx);
                }

                // Function JITs with AWAIT are rejected until exact deopt
                // frame states exist. Keep this sentinel check as a safety
                // net for future emission paths that might violate the gate.
                if (jitCtx.pendingException
                    && isStashedOSRDeopt(jitCtx.pendingException))
                {
                    // Ordinary function JIT frames have no deopt snapshot.
                    // Restarting from bytecode entry would repeat all effects
                    // before AWAIT, so fail closed if the canCompile() guard is
                    // ever bypassed by a future emitter or inliner.
                    std::rethrow_exception(jitCtx.pendingException);
                }

                if (jitCtx.hasReturnValue)
                    stackManager->push(jitCtx.returnValue);

                stats.functionCalls++;
                return;
            }
        }
        functionExecutor->handleCall(instr);
    }

    void VirtualMachine::executeCallFastWithJit(
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (jitEnabled && jitCodeCache && jitNativeDepth < MAX_JIT_NATIVE_DEPTH
            && !inJitFallbackInterpreter)
        {
            const auto* activeProgram = executionCtx ? executionCtx->program : program;
            const auto* funcMeta = activeProgram->getFunctionByIndex(instr.inlineOperands[0]);
            if (funcMeta) {
                auto jitCode = jitCodeCache->lookup(
                    activeProgram->getProgramId(), funcMeta->mangledName);
                if (jitCode)
                {
                    size_t argCount = instr.inlineOperands[1];

                    value::SmallArgsBuffer args(argCount);
                    for (size_t i = argCount; i > 0; --i)
                        args[i - 1] = stackManager->pop();

                    jit::JitContext jitCtx{};
                    jitCtx.args = args.data();
                    jitCtx.argCount = args.size();
                    jitCtx.hasReturnValue = false;
                    jitCtx.program = activeProgram;
                    jitCtx.stackManager = stackManager.get();
                    jitCtx.environment = environment.get();
                    jitCtx.vm = this;
                    jitCtx.jitCodeCache = jitCodeCache.get();
                    jitCtx.icTable = inlineCacheTable.get();

                    size_t sepPos = funcMeta->name.find("::");
                    if (sepPos != std::string::npos)
                        jitCtx.callingClassName = funcMeta->name.substr(0, sepPos);

                    {
                        jit::ScopedJitNativeDepth nativeFrame(*this);
                        jitCode(&jitCtx);
                    }

                    // MYT-268: see executeCallWithJit for rationale.
                    if (jitCtx.pendingException
                        && isStashedOSRDeopt(jitCtx.pendingException))
                    {
                        std::rethrow_exception(jitCtx.pendingException);
                    }

                    if (jitCtx.hasReturnValue)
                        stackManager->push(jitCtx.returnValue);

                    stats.functionCalls++;
                    return;
                }
            }
        }
        functionExecutor->handleCallFast(instr);
    }

    void VirtualMachine::invalidateInlinedFunctionCallers(
        bytecode::FunctionNameHandle callee)
    {
        if (program) invalidateInlinedFunctionCallers(*program, callee);
    }

    void VirtualMachine::invalidateInlinedFunctionCallers(
        const bytecode::BytecodeProgram& calleeProgram,
        bytecode::FunctionNameHandle callee)
    {
        if (!jitCodeCache) return;
        auto callers = jitCodeCache->invalidatedInlineCallersOf(
            calleeProgram.getProgramId(), callee);
        if (callers.empty()) return;

        auto* icTable = inlineCacheTable.get();
        for (const auto& caller : callers)
        {
            // JitCodeCache::invalidate releases the native code memory and
            // returns the JitFunction pointer that was freed. The MYT-315
            // contract requires us to scrub every IC table that may hold
            // that pointer.
            jit::JitFunction removed = jitCodeCache->invalidate(caller);
            if (!removed) continue;
            const void* removedPtr = reinterpret_cast<const void*>(removed);
            if (icTable)
            {
                icTable->clearCachedJitForFunction(removedPtr);
            }
            // MYT-322: also scrub free-function IC side-tables on every
            // loaded program. A library callee invalidated here may have
            // cachedJitFnPtr entries in caller programs' CachedInstructionState
            // maps; those tables live per-BytecodeProgram, not in the
            // global ICTable, so we must visit them all to avoid jumping
            // into freed native code on the next warm dispatch.
            for (const auto* prog : getLoadedPrograms())
            {
                if (prog) prog->clearCachedJitFnPtrFor(removedPtr);
            }
            // A caller F that inlined `callee` may itself have been inlined
            // into a third caller G. We don't have F's handle directly here
            // — the next compile that pastes F into G will register a fresh
            // edge under F's handle. For correctness today we rely on the
            // current invalidate target chain (PluginLoader walks all
            // unbound names; REPL redefinition passes the redefined
            // function's handle). If transitive eviction becomes necessary,
            // resolve callerName -> handle via internFrameName and recurse.
        }
    }
}
