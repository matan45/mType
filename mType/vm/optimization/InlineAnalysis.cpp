#include "InlineAnalysis.hpp"
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

                case OpCode::LOAD_UPVALUE:
                case OpCode::STORE_UPVALUE:
                case OpCode::CLOSE_UPVALUE:
                    return InlineDecision::HAS_UPVALUES;

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
                case OpCode::LAMBDA_INVOKE:
                    return InlineDecision::HAS_NESTED_CALL;

                // MYT-167 (F-e): ValueObject receivers are read-only inline
                // eligible. Field writes require the COW slot rewrite in
                // setFieldOnValueObject which cannot be naively lifted into
                // an inlined body; reject such callees for ValueObject sites.
                case OpCode::SET_FIELD:
                case OpCode::INLINE_SET_FIELD:
                    if (isValueObjectReceiver)
                        return InlineDecision::VALUE_OBJECT_WRITES_FIELDS;
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
        const std::string& currentCompilingFn)
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
        const std::string& currentCompilingFn)
    {
        if (!entry.shape || !entry.funcMetadata)
            return InlineDecision::UNKNOWN_SHAPE;

        const auto* callee = static_cast<const BytecodeProgram::FunctionMetadata*>(
            entry.funcMetadata);

        return checkCalleeEligibility(program, callee, entry.receiverIsValueObject,
                                       entry.qualifiedName, currentCompilingFn);
    }

    InlineDecision checkInlineEligibility(
        const BytecodeProgram& program,
        const MethodInlineCache& cache,
        const std::string& currentCompilingFn,
        size_t currentInlineDepth)
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

        size_t combinedSize = 0;
        for (uint8_t i = 0; i < cache.entryCount; ++i)
        {
            auto d = checkEntryEligibility(program, cache.entries[i], currentCompilingFn);
            if (d != InlineDecision::INLINE)
                return d;

            const auto* callee = static_cast<const BytecodeProgram::FunctionMetadata*>(
                cache.entries[i].funcMetadata);
            combinedSize += callee->instructionCount;
        }

        // MYT-165: combined-body cap prevents code-cache blowup on maximally
        // polymorphic sites. IC_MAX_POLYMORPHIC_ENTRIES × INLINE_SIZE_LIMIT
        // = 4 × 16 = 64 ops across all shape bodies.
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
            case InlineDecision::CALLEE_NATIVE:              return "CALLEE_NATIVE";
            case InlineDecision::CALLEE_NOT_FOUND:           return "CALLEE_NOT_FOUND";
            case InlineDecision::HAS_UNSUPPORTED_OPCODE:     return "HAS_UNSUPPORTED_OPCODE";
        }
        return "UNKNOWN";
    }
}
