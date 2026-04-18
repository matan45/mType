#include "InlineAnalysis.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../bytecode/OpCode.hpp"
#include "../jit/ic/InlineCacheTypes.hpp"

namespace vm::optimization
{
    using vm::bytecode::BytecodeProgram;
    using vm::bytecode::OpCode;
    using vm::jit::ic::ICState;
    using vm::jit::ic::MethodInlineCache;

    // Scan callee bytecode for any opcode that disqualifies F-a inlining.
    // Order matches the Decision enum preference — the first disqualifying
    // opcode wins, so a callee that both branches and awaits reports
    // HAS_INTERNAL_JUMPS (the more relaxable restriction).
    static InlineDecision scanCalleeOpcodes(
        const BytecodeProgram& program,
        const BytecodeProgram::FunctionMetadata& callee)
    {
        const size_t end = callee.startOffset + callee.instructionCount;
        for (size_t ip = callee.startOffset; ip < end; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            switch (instr.opcode)
            {
                case OpCode::JUMP:
                case OpCode::JUMP_IF_FALSE:
                case OpCode::JUMP_IF_TRUE:
                case OpCode::JUMP_IF_NULL:
                case OpCode::JUMP_BACK:
                case OpCode::JUMP_IF_FALSE_OR_POP:
                case OpCode::JUMP_IF_TRUE_OR_POP:
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
                case OpCode::CALL_METHOD:
                case OpCode::CALL_STATIC:
                case OpCode::INVOKE:
                case OpCode::LAMBDA_INVOKE:
                    return InlineDecision::HAS_NESTED_CALL;

                default:
                    break;
            }
        }
        return InlineDecision::INLINE;
    }

    InlineDecision checkInlineEligibility(
        const BytecodeProgram& program,
        const MethodInlineCache& cache,
        const std::string& currentCompilingFn,
        size_t currentInlineDepth)
    {
        if (currentInlineDepth >= INLINE_DEPTH_LIMIT)
            return InlineDecision::DEPTH_EXCEEDED;

        if (cache.state != ICState::MONOMORPHIC)
            return InlineDecision::IC_NOT_MONOMORPHIC;

        if (cache.entryCount == 0)
            return InlineDecision::UNKNOWN_SHAPE;

        const auto& entry = cache.entries[0];
        if (!entry.shape || !entry.funcMetadata)
            return InlineDecision::UNKNOWN_SHAPE;

        if (entry.receiverIsValueObject)
            return InlineDecision::VALUE_OBJECT_RECEIVER;

        const auto* callee = static_cast<const BytecodeProgram::FunctionMetadata*>(
            entry.funcMetadata);

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
        // methods (RETURN without a value) would leave the caller's operand
        // stack out of sync with the generic path (which always pushes a
        // monostate via emitReturnValueCopyBoxed). Reject until F-b handles
        // the void-return case.
        if (callee->returnType == "void")
            return InlineDecision::CALLEE_NOT_FOUND;

        // Self-recursive inlining is forbidden — would chase unbounded emit.
        if (!currentCompilingFn.empty() &&
            (callee->mangledName == currentCompilingFn ||
             callee->name == currentCompilingFn ||
             entry.qualifiedName == currentCompilingFn))
            return InlineDecision::SELF_RECURSIVE;

        return scanCalleeOpcodes(program, *callee);
    }
}
