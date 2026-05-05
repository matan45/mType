#include "SuperinstructionPattern.hpp"
#include <cstddef>

namespace vm::optimization::patterns
{
    using bytecode::OpCode;
    using Instruction = bytecode::BytecodeProgram::Instruction;

    bool SuperinstructionPattern::isStructural(OpCode op)
    {
        // Meta/profile/exception-region markers and LOOP markers — fusing
        // across one would corrupt the exception table, profiling state, or
        // loop bounds. Also reject NOP to avoid pointless fusion with padding.
        switch (op)
        {
        case OpCode::LINE:
        case OpCode::SOURCE_FILE:
        case OpCode::PROFILE_ENTER:
        case OpCode::PROFILE_EXIT:
        case OpCode::TRY_BEGIN:
        case OpCode::TRY_END:
        case OpCode::CATCH:
        case OpCode::FINALLY:
        case OpCode::LOOP_START:
        case OpCode::LOOP_END:
        case OpCode::NOP:
            return true;
        default:
            return false;
        }
    }

    SuperinstructionPattern::Kind SuperinstructionPattern::classify(
        const bytecode::BytecodeProgram& program, size_t offset) const
    {
        const size_t count = program.getInstructionCount();
        if (offset + 1 >= count) return Kind::None;

        const auto& a = program.getInstruction(offset);
        const auto& b = program.getInstruction(offset + 1);

        if (isStructural(a.opcode) || isStructural(b.opcode)) return Kind::None;

        // Pair: LOAD_LOCAL + STORE_LOCAL.
        // STORE_LOCAL can carry an optional operand[1] = varName index for
        // shared-frame late-binding (used when a lambda captures the local
        // at a later point in execution). The fused opcode only packs the
        // slots, so reject the fuse if the varName is present — keeping the
        // unfused path preserves lambda-capture semantics bit-identically.
        // Both sides require exactly one operand: LOAD_LOCAL never legitimately
        // carries extras, and tightening the check is defensive against any
        // future opcode-format drift.
        if (a.opcode == OpCode::LOAD_LOCAL && b.opcode == OpCode::STORE_LOCAL &&
            a.operands.size() == 1 && b.operands.size() == 1)
        {
            return Kind::LoadStoreLocal;
        }

        // Pair: LOAD_LOCAL + GET_FIELD. GET_FIELD always carries exactly one
        // operand (field-name index); anything else would indicate a format
        // change the fused executor hasn't been updated for. LOAD_LOCAL
        // likewise must carry exactly one operand.
        if (a.opcode == OpCode::LOAD_LOCAL && b.opcode == OpCode::GET_FIELD &&
            a.operands.size() == 1 && b.operands.size() == 1)
        {
            return Kind::LoadGetField;
        }

        // Pair: ADD_INT + STORE_LOCAL (varName-free form only, same reason as
        // LoadStoreLocal — fusion drops the shared-frame late-binding operand).
        if (a.opcode == OpCode::ADD_INT && b.opcode == OpCode::STORE_LOCAL &&
            b.operands.size() == 1)
        {
            return Kind::AddIntStoreLocal;
        }

        // Pair: OBJECT_TO_VALUE + CREATE_PROMISE. Hot on every async function
        // that returns a primitive-boxed value (Int/Float/Bool/etc.) — the
        // compiler emits NEW_VALUE_OBJECT, OBJECT_TO_VALUE, CREATE_PROMISE,
        // RETURN_VALUE for `return new Int(x)` style. Both opcodes are pure
        // stack ops with no operands.
        if (a.opcode == OpCode::OBJECT_TO_VALUE && b.opcode == OpCode::CREATE_PROMISE &&
            a.operands.empty() && b.operands.empty())
        {
            return Kind::ObjectToValueCreatePromise;
        }

        // Pair: CREATE_PROMISE + RETURN_VALUE. Closing pair of every async
        // value-returning function. Pure stack ops, no operands. Safe — no
        // suspend semantics on either side.
        if (a.opcode == OpCode::CREATE_PROMISE && b.opcode == OpCode::RETURN_VALUE &&
            a.operands.empty() && b.operands.empty())
        {
            return Kind::CreatePromiseReturnValue;
        }

        // Triples
        if (offset + 2 >= count) return Kind::None;
        const auto& c = program.getInstruction(offset + 2);
        if (isStructural(c.opcode)) return Kind::None;

        if (a.opcode == OpCode::LOAD_LOCAL && b.opcode == OpCode::LOAD_LOCAL &&
            a.operands.size() == 1 && b.operands.size() == 1)
        {
            switch (c.opcode)
            {
            case OpCode::ADD_INT: return Kind::LoadLoadAddInt;
            case OpCode::SUB_INT: return Kind::LoadLoadSubInt;
            case OpCode::MUL_INT: return Kind::LoadLoadMulInt;
            default: break;
            }
        }
        return Kind::None;
    }

    bool SuperinstructionPattern::matches(const bytecode::BytecodeProgram& program,
                                          size_t offset,
                                          const analysis::ControlFlowAnalyzer& /*cfg*/) const
    {
        // NOTE: classify() runs again inside apply() for the same (program,
        // offset) pair. A one-slot (program*, offset) memo cache looks
        // attractive, but it's UNSOUND under program mutation: the peephole
        // pass mutates the instruction vector after each successful apply()
        // (shrinking it by originalLength - 1), and a later classify() call
        // at a numerically identical offset in the shrunk program would hit
        // the cache with stale Kind data and drive apply() to emit a
        // replacement based on instructions that no longer live at that
        // offset. Safely caching would require keying on the instruction
        // contents (opcodes + operands in the window), which is most of
        // classify()'s work — net loss. Today classify is cheap (two opcode
        // comparisons + a couple operand-count checks), so the double call
        // is negligible. If this becomes a hot path (e.g. multi-instruction
        // window analysis is added), extend OptimizationPattern's interface
        // to pass a match token from matches() → apply() instead.
        Kind kind = classify(program, offset);
        if (kind == Kind::None) return false;

        const bool isTriple = kind == Kind::LoadLoadAddInt ||
                              kind == Kind::LoadLoadSubInt ||
                              kind == Kind::LoadLoadMulInt;

        // Interior offsets must not be jump / try / function-entry targets —
        // a jump landing inside the fused range would skip the first op and
        // corrupt the operand stack. `offset` itself is also rejected so we
        // never relocate a function's startOffset via the shrink fixup path.
        if (program.isFusionUnsafeTarget(offset)) return false;
        if (program.isFusionUnsafeTarget(offset + 1)) return false;
        if (isTriple && program.isFusionUnsafeTarget(offset + 2)) return false;

        // Defensive: skip sites that already have runtime-fused side-table
        // state (MYT-198). Peephole runs at compile time before any runtime
        // rewrite, so this shouldn't fire — but bail rather than fight.
        if (program.findCachedState(offset) || program.findCachedState(offset + 1))
        {
            return false;
        }
        if (isTriple && program.findCachedState(offset + 2)) return false;

        return true;
    }

    OptimizationPattern::Replacement SuperinstructionPattern::apply(
        const bytecode::BytecodeProgram& program, size_t offset) const
    {
        Kind kind = classify(program, offset);

        Replacement result;
        switch (kind)
        {
        case Kind::LoadStoreLocal:
        {
            const auto& a = program.getInstruction(offset);
            const auto& b = program.getInstruction(offset + 1);
            result.originalLength = 2;
            result.instructions.emplace_back(
                OpCode::LOAD_STORE_LOCAL, a.operands[0], b.operands[0]);
            break;
        }
        case Kind::LoadGetField:
        {
            const auto& a = program.getInstruction(offset);
            const auto& b = program.getInstruction(offset + 1);
            result.originalLength = 2;
            result.instructions.emplace_back(
                OpCode::LOAD_GET_FIELD, a.operands[0], b.operands[0]);
            break;
        }
        case Kind::AddIntStoreLocal:
        {
            const auto& b = program.getInstruction(offset + 1);
            result.originalLength = 2;
            result.instructions.emplace_back(
                OpCode::ADD_INT_STORE_LOCAL, b.operands[0]);
            break;
        }
        case Kind::ObjectToValueCreatePromise:
        {
            result.originalLength = 2;
            result.instructions.emplace_back(OpCode::OBJECT_TO_VALUE_CREATE_PROMISE);
            break;
        }
        case Kind::CreatePromiseReturnValue:
        {
            result.originalLength = 2;
            result.instructions.emplace_back(OpCode::CREATE_PROMISE_RETURN_VALUE);
            break;
        }
        case Kind::LoadLoadAddInt:
        {
            const auto& a = program.getInstruction(offset);
            const auto& b = program.getInstruction(offset + 1);
            result.originalLength = 3;
            result.instructions.emplace_back(
                OpCode::LOAD_LOAD_ADD_INT, a.operands[0], b.operands[0]);
            break;
        }
        case Kind::LoadLoadSubInt:
        {
            const auto& a = program.getInstruction(offset);
            const auto& b = program.getInstruction(offset + 1);
            result.originalLength = 3;
            result.instructions.emplace_back(
                OpCode::LOAD_LOAD_SUB_INT, a.operands[0], b.operands[0]);
            break;
        }
        case Kind::LoadLoadMulInt:
        {
            const auto& a = program.getInstruction(offset);
            const auto& b = program.getInstruction(offset + 1);
            result.originalLength = 3;
            result.instructions.emplace_back(
                OpCode::LOAD_LOAD_MUL_INT, a.operands[0], b.operands[0]);
            break;
        }
        case Kind::None:
            // matches() returned true for an applicable kind; this branch
            // should be unreachable. Leave originalLength=0 so the optimizer
            // treats this as a no-op and moves on.
            break;
        }
        return result;
    }

} // namespace vm::optimization::patterns
