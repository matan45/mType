#include "InlineAnalysis.hpp"
#include <cstdint>
#include "../bytecode/BytecodeProgram.hpp"
#include "../bytecode/OpCode.hpp"
#include "../jit/ic/InlineCacheTypes.hpp"

namespace vm::optimization
{
    using vm::bytecode::BytecodeProgram;
    using vm::bytecode::OpCode;
    using vm::jit::ic::ICState;
    using vm::jit::ic::MethodICEntry;
    using vm::jit::ic::MethodInlineCache;

    // Scan callee bytecode for any opcode that disqualifies F-b inlining.
    // Order matches the Decision enum preference — the first disqualifying
    // opcode wins.
    //
    // F-b (MYT-164) unlocks internal control flow (JUMP/JUMP_IF_FALSE/TRUE/
    // JUMP_BACK/JUMP_IF_FALSE_OR_POP/JUMP_IF_TRUE_OR_POP) via per-frame
    // localJumpLabels in tryEmitInlinedMethodCall, and unlocks nested inline
    // for a single CALL_METHOD (the recursive path in tryEmitInlinedMethodCall
    // enforces INLINE_DEPTH_LIMIT = 2; a non-inlineable nested site simply
    // falls through to emitCallMethodOpGeneric). JUMP_IF_NULL stays blocked
    // until its primitive emitter lands; the other CALL_* / INVOKE / LAMBDA_*
    // opcodes stay blocked — their return-value paths are not wired through
    // the inline operand-stack contract.
    static InlineDecision scanCalleeOpcodes(
        const BytecodeProgram& program,
        const BytecodeProgram::FunctionMetadata& callee,
        bool isValueObjectReceiver)
    {
        const size_t end = callee.startOffset + callee.instructionCount;
        for (size_t ip = callee.startOffset; ip < end; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            switch (instr.opcode)
            {
                // MYT-164: JUMP_IF_NULL has no JIT emitter in F-b's reach; keep
                // it on the bailout list until a primitive emitter is wired.
                case OpCode::JUMP_IF_NULL:
                    return InlineDecision::HAS_INTERNAL_JUMPS;

                case OpCode::TRY_BEGIN:
                case OpCode::TRY_END:
                case OpCode::CATCH:
                case OpCode::FINALLY:
                case OpCode::THROW:
                    return InlineDecision::HAS_TRY_CATCH;

                case OpCode::AWAIT:
                case OpCode::CREATE_PROMISE:
                case OpCode::PROMISE_RESOLVE:
                    return InlineDecision::HAS_ASYNC;

                // MYT-B1: HAS_UPVALUES branch removed — LOAD/STORE/CLOSE_UPVALUE
                // opcodes no longer exist. Lambda capture flows through
                // SharedStackFrame parent chains, which the inliner handles
                // via the SharedStackFrame visibility check elsewhere.

                case OpCode::SUPER_INVOKE:
                case OpCode::SUPER_CONSTRUCTOR:
                case OpCode::SUPER_GET_FIELD:
                case OpCode::SUPER_SET_FIELD:
                case OpCode::GET_SUPER:
                    return InlineDecision::HAS_SUPER_CALL;

                case OpCode::CALL:
                case OpCode::CALL_FAST:
                case OpCode::CALL_STATIC:
                case OpCode::INVOKE:
                    return InlineDecision::HAS_NESTED_CALL;

                // MYT-167 (F-e) original: ValueObject receivers were read-only
                // inline eligible because the inlined SET_FIELD path's CoW
                // rewrite in setFieldOnValueObject targets the operand-stack
                // slot of LOAD_LOCAL, not the inlined-frame local-0, so the
                // mutation was silently dropped.
                //
                // MYT-346: relaxed. The inliner now materialises a temp
                // ObjectInstance for value-class receivers (see
                // jit_materialise_value_receiver_into_local) so local-0 holds
                // an OBJECT-tagged Value before the body emits. Once
                // materialised, the inlined GET_FIELD / SET_FIELD opcodes
                // operate on a normal ObjectInstance and mutation works.
                //
                // SET_FIELD_CACHED is the IC-promoted variant that emerges in
                // the callee body after the first interpreted call (the
                // temp-instance dispatch populates the field IC and the MONO
                // promotion rewrites the in-place opcode). This was the actual
                // bug path — scanCalleeOpcodes previously missed it because it
                // only watched SET_FIELD / INLINE_SET_FIELD. SET_FIELD_TYPED
                // and SET_FIELD_FAST are listed defensively (currently emitted
                // only from ctor inline-init / unused respectively).
                case OpCode::SET_FIELD:
                case OpCode::INLINE_SET_FIELD:
                case OpCode::SET_FIELD_CACHED:
                case OpCode::SET_FIELD_TYPED:
                case OpCode::SET_FIELD_FAST:
                    if (isValueObjectReceiver)
                        return InlineDecision::INLINE_VALUE_REQUIRES_MATERIALISATION;
                    break;

                // MYT-185: STRING_BUILD has no JIT emitter. The emission loop
                // in tryEmitInlinedMethodCall silently skips unknown opcodes
                // (emitObjectOps's default case), leaving the compile-time
                // stackDepth/slotTypes out of sync with runtime state. Any
                // callee containing STRING_BUILD (e.g. `return a + ":" + b`
                // where the compiler fuses concats into a single build) must
                // bail to the generic slow path until a JIT handler lands.
                case OpCode::STRING_BUILD:
                    return InlineDecision::HAS_UNSUPPORTED_OPCODE;

                // MYT-208: inlining a callee that contains NEW_STACK dissolves
                // the per-call frame boundary that escape analysis relies on.
                // Stack-promoted allocations would push to the *caller* frame's
                // stackObjects (which can outlive the inlined call by orders of
                // magnitude — e.g. 2M iterations of an outer loop), filling the
                // per-frame cap after ~32 allocs and forcing every subsequent
                // promotion to fall back to the heap path. Rejecting inlining
                // here keeps each invocation in its own frame, where the
                // analyzer's "lifetime bounded by frame" guarantee actually
                // holds and stackObjects releases on RETURN. This is the key
                // gate that makes nested-helper benchmarks (object_alloc_nested.mt)
                // realise the STACK_OBJECT win.
                case OpCode::NEW_STACK:
                    return InlineDecision::HAS_UNSUPPORTED_OPCODE;

                // MYT-210: the ARRAY_*_LOCAL fused variants bake a raw local
                // slot index into the instruction and read it via
                // Mem(localsBase, localSlot * localStride) — they do NOT
                // consult s.inlineLocalsBase like LOAD_LOCAL / STORE_LOCAL do.
                // For an inlined callee, the callee's local-0 (first arg)
                // actually lives at (0 + inlineLocalsBase) * localStride, so
                // the baked index reads garbage from the caller's frame.
                // Reject until the _LOCAL emitters respect inlineLocalsBase.
                case OpCode::ARRAY_GET_INT_LOCAL:
                case OpCode::ARRAY_SET_INT_LOCAL:
                case OpCode::ARRAY_LENGTH_LOCAL:
                    return InlineDecision::HAS_UNSUPPORTED_OPCODE;

                // MYT-274 Phase 2 v2: structural-equality fused opcodes
                // have a JIT emit path (helper-call to jit_struct_hash_int /
                // jit_struct_eq_int in JitCompiler_Objects.cpp) and are
                // accepted as inlineable here. The body containing one of
                // these is small (LOAD_LOCAL + fused op + RETURN_VALUE for
                // hashCode; +1 LOAD_LOCAL for equals) — well under
                // INLINE_SIZE_LIMIT, no nested method calls or unsupported
                // jumps, so all the standard eligibility checks downstream
                // pass without further special-casing.

                default:
                    break;
            }
        }
        return InlineDecision::INLINE;
    }

    // MYT-210: shared per-callee gate, reused by both the method-IC path
    // (one entry at a time) and the plain-CALL path. Checks that depend
    // purely on the callee's metadata + the receiver's runtime classification
    // (boolean instead of MethodICEntry, so plain CALL can pass `false`).
    static InlineDecision checkCalleeEligibility(
        const BytecodeProgram& program,
        const BytecodeProgram::FunctionMetadata* callee,
        bool receiverIsValueObject,
        const std::string& qualifiedName,
        const std::string& currentCompilingFn,
        bool isOSRCompilation = false)
    {
        if (!callee)
            return InlineDecision::UNKNOWN_SHAPE;

        if (callee->isNative)
            return InlineDecision::CALLEE_NATIVE;

        if (callee->isAsync)
            return InlineDecision::HAS_ASYNC;

        if (!callee->exceptionTable.getEntries().empty())
            return InlineDecision::HAS_TRY_CATCH;

        if (callee->instructionCount == 0)
            return InlineDecision::CALLEE_NOT_FOUND;

        if (callee->instructionCount > INLINE_SIZE_LIMIT)
            return InlineDecision::CALLEE_TOO_BIG;

        // F-a emits only the RETURN_VALUE substitution; void-returning
        // callees (RETURN without a value) would leave the caller's operand
        // stack out of sync with the generic path (which always pushes a
        // monostate via emitReturnValueCopyBoxed). Reject until a void-return
        // path is added (out of scope for MYT-210).
        if (callee->returnType == "void")
            return InlineDecision::CALLEE_NOT_FOUND;

        // Self-recursive inlining is forbidden — would chase unbounded emit.
        // For plain CALL the qualifiedName is empty; the mangled/name compare
        // still catches direct self-recursion on the function table.
        //
        // MYT-251: in OSR (`isOSRCompilation == true`) there is no static
        // caller name — the OSR'd loop body belongs to whatever function
        // owns the loop, but the JIT compiles the loop range in isolation
        // and currentCompilingFn is intentionally left empty. The check is
        // therefore a no-op in OSR; emit-time depth bounding (INLINE_DEPTH_LIMIT
        // and inlineStack.size()) keeps recursion finite.
        (void)isOSRCompilation;
        if (!currentCompilingFn.empty() &&
            (callee->mangledName == currentCompilingFn ||
             callee->name == currentCompilingFn ||
             qualifiedName == currentCompilingFn))
            return InlineDecision::SELF_RECURSIVE;

        return scanCalleeOpcodes(program, *callee, receiverIsValueObject);
    }

    // Per-entry eligibility: checks that apply independently to one IC entry's
    // callee. MONO uses this once; POLY (MYT-165) loops over every entry and
    // requires all to return INLINE. MYT-210: thin wrapper over the shared
    // checkCalleeEligibility helper.
    static InlineDecision checkEntryEligibility(
        const BytecodeProgram& program,
        const MethodICEntry& entry,
        const std::string& currentCompilingFn,
        bool isOSRCompilation = false)
    {
        if (!entry.shape || !entry.funcMetadata)
            return InlineDecision::UNKNOWN_SHAPE;

        const auto* callee = static_cast<const BytecodeProgram::FunctionMetadata*>(
            entry.funcMetadata);

        return checkCalleeEligibility(program, callee, entry.receiverIsValueObject,
                                       entry.qualifiedName, currentCompilingFn,
                                       isOSRCompilation);
    }

    InlineDecision checkInlineEligibility(
        const BytecodeProgram& program,
        const MethodInlineCache& cache,
        const std::string& currentCompilingFn,
        size_t currentInlineDepth,
        bool isOSRCompilation,
        std::array<InlineDecision, vm::jit::ic::IC_MAX_POLYMORPHIC_ENTRIES>*
            perEntryDecisions)
    {
        if (currentInlineDepth >= INLINE_DEPTH_LIMIT)
            return InlineDecision::DEPTH_EXCEEDED;

        // MYT-165: accept POLYMORPHIC in addition to MONOMORPHIC. The emitter
        // branches on cache.state to pick the MONO (single guard + body) or
        // POLY (chained guards + per-shape bodies) emission path. Enum name
        // kept for continuity — its meaning is now "IC not in an inline-
        // eligible state".
        if (cache.state != ICState::MONOMORPHIC &&
            cache.state != ICState::POLYMORPHIC)
            return InlineDecision::IC_NOT_MONOMORPHIC;

        if (cache.entryCount == 0)
            return InlineDecision::UNKNOWN_SHAPE;

        // MYT-173 follow-up: per-entry eligibility. Walk every entry and
        // record the decision; the site is "INLINE" if at least one entry
        // is inlineable. emitInlinedMethodCallPoly routes the rest through
        // the per-shape helper branch (jit_call_method_ic) — the guard
        // chain stays the same width, only the body emission switches.
        // Pre-change every entry had to pass; one HAS_TRY_CATCH or
        // CALLEE_TOO_BIG entry forfeited the entire site to the helper.
        size_t combinedSize = 0;
        bool anyInlineable = false;
        InlineDecision firstReject = InlineDecision::INLINE;
        for (uint8_t i = 0; i < cache.entryCount; ++i)
        {
            auto d = checkEntryEligibility(program, cache.entries[i], currentCompilingFn,
                                            isOSRCompilation);
            if (perEntryDecisions)
                (*perEntryDecisions)[i] = d;

            // MYT-346: INLINE_VALUE_REQUIRES_MATERIALISATION counts as
            // inlineable for the site-level decision — the emitter handles
            // the value-class temp materialisation per-arm before the body.
            if (d == InlineDecision::INLINE ||
                d == InlineDecision::INLINE_VALUE_REQUIRES_MATERIALISATION)
            {
                anyInlineable = true;
                const auto* callee = static_cast<const BytecodeProgram::FunctionMetadata*>(
                    cache.entries[i].funcMetadata);
                combinedSize += callee->instructionCount;
            }
            else if (firstReject == InlineDecision::INLINE)
            {
                firstReject = d;
            }
        }

        if (!anyInlineable)
            return firstReject;

        // MYT-165 / MYT-173 follow-up: combined-body cap prevents code-cache
        // blowup. Now measured only over inlineable entries (helper-routed
        // entries pay no code-cache space beyond the guard + helper-call
        // stub). Cap remains IC_MAX_POLYMORPHIC_ENTRIES × INLINE_SIZE_LIMIT.
        if (combinedSize > INLINE_SIZE_LIMIT * vm::jit::ic::IC_MAX_POLYMORPHIC_ENTRIES)
            return InlineDecision::CALLEE_TOO_BIG;

        return InlineDecision::INLINE;
    }

    // MYT-210: plain-CALL / CALL_FAST eligibility. No IC, no combined-size cap
    // (single statically-known callee), no receiver shape — just depth gate and
    // the shared per-callee checks. receiverIsValueObject = false because plain
    // functions have no `this`.
    InlineDecision checkFunctionInlineEligibility(
        const BytecodeProgram& program,
        const BytecodeProgram::FunctionMetadata& callee,
        const std::string& currentCompilingFn,
        size_t currentInlineDepth)
    {
        if (currentInlineDepth >= INLINE_DEPTH_LIMIT)
            return InlineDecision::DEPTH_EXCEEDED;

        return checkCalleeEligibility(program, &callee,
                                       /*receiverIsValueObject=*/false,
                                       /*qualifiedName=*/std::string(),
                                       currentCompilingFn);
    }

    // MYT-210: human-readable name for --jit-stats output. Mirrors the shape of
    // osrBailoutReasonName (one switch, no map lookup).
    const char* inlineDecisionName(InlineDecision d)
    {
        switch (d)
        {
            case InlineDecision::INLINE:                     return "INLINE";
            case InlineDecision::CALLEE_TOO_BIG:             return "CALLEE_TOO_BIG";
            case InlineDecision::HAS_TRY_CATCH:              return "HAS_TRY_CATCH";
            case InlineDecision::HAS_ASYNC:                  return "HAS_ASYNC";
            case InlineDecision::HAS_UPVALUES:               return "HAS_UPVALUES";
            case InlineDecision::HAS_SUPER_CALL:             return "HAS_SUPER_CALL";
            case InlineDecision::HAS_NESTED_CALL:            return "HAS_NESTED_CALL";
            case InlineDecision::HAS_INTERNAL_JUMPS:         return "HAS_INTERNAL_JUMPS";
            case InlineDecision::SELF_RECURSIVE:             return "SELF_RECURSIVE";
            case InlineDecision::DEPTH_EXCEEDED:             return "DEPTH_EXCEEDED";
            case InlineDecision::IC_NOT_MONOMORPHIC:         return "IC_NOT_MONOMORPHIC";
            case InlineDecision::UNKNOWN_SHAPE:              return "UNKNOWN_SHAPE";
            case InlineDecision::VALUE_OBJECT_RECEIVER:      return "VALUE_OBJECT_RECEIVER";
            case InlineDecision::VALUE_OBJECT_WRITES_FIELDS: return "VALUE_OBJECT_WRITES_FIELDS";
            case InlineDecision::INLINE_VALUE_REQUIRES_MATERIALISATION: return "INLINE_VALUE_REQUIRES_MATERIALISATION";
            case InlineDecision::CALLEE_NATIVE:              return "CALLEE_NATIVE";
            case InlineDecision::CALLEE_NOT_FOUND:           return "CALLEE_NOT_FOUND";
            case InlineDecision::HAS_UNSUPPORTED_OPCODE:     return "HAS_UNSUPPORTED_OPCODE";
        }
        return "UNKNOWN";
    }
}
