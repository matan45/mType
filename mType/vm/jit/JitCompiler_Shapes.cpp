#include "JitCompiler_Shapes.hpp"
#include "JitCompiler.hpp"
#include <cstdint>
#include <string>

namespace vm::jit
{
    using OpCode = bytecode::OpCode;

    namespace
    {
        // MYT-302 follow-up telemetry: see JitCompiler::getLoopConditionGuardHits.
        // Plain uint64_t — JIT is single-threaded.
        uint64_t g_loopConditionGuardHits = 0;

        bool isArrayAccessOpcode(OpCode opcode)
        {
            switch (opcode)
            {
                case OpCode::ARRAY_GET:
                case OpCode::ARRAY_GET_ALIAS:
                case OpCode::ARRAY_SET:
                case OpCode::ARRAY_LENGTH:
                case OpCode::ARRAY_GET_INT:
                case OpCode::ARRAY_SET_INT:
                case OpCode::ARRAY_GET_FIELD:
                case OpCode::ARRAY_SET_FIELD:
                case OpCode::ARRAY_LENGTH_LOCAL:
                case OpCode::ARRAY_GET_INT_LOCAL:
                case OpCode::ARRAY_SET_INT_LOCAL:
                    return true;
                default:
                    return false;
            }
        }

        bool localSlotOpcode(OpCode opcode,
                              const bytecode::BytecodeProgram::Instruction& instr,
                              size_t& outSlot)
        {
            switch (opcode)
            {
                case OpCode::LOAD_LOCAL:
                case OpCode::STORE_LOCAL:
                case OpCode::LOAD_LOCAL_INT:
                case OpCode::LOAD_LOCAL_FLOAT:
                case OpCode::LOAD_LOCAL_BOOL:
                case OpCode::LOAD_LOCAL_BOXED_INST:
                case OpCode::STORE_LOCAL_INT:
                case OpCode::STORE_LOCAL_FLOAT:
                case OpCode::STORE_LOCAL_BOOL:
                case OpCode::STORE_LOCAL_BOXED_INST:
                    if (instr.numOperands() == 0)
                        return false;
                    outSlot = instr.inlineOperands[0];
                    return true;
                default:
                    return false;
            }
        }

        bool isVariableAccessOpcode(OpCode opcode)
        {
            switch (opcode)
            {
                case OpCode::LOAD_VAR:
                case OpCode::STORE_VAR:
                case OpCode::LOAD_VAR_CACHED:
                case OpCode::STORE_VAR_CACHED:
                    return true;
                default:
                    return false;
            }
        }

        bool isPushIntValue(const bytecode::BytecodeProgram& program,
                             const bytecode::BytecodeProgram::Instruction& instr,
                             int64_t expected)
        {
            if (instr.opcode != OpCode::PUSH_INT || instr.numOperands() == 0)
                return false;
            const auto index = static_cast<size_t>(instr.inlineOperands[0]);
            const auto& integers = program.getConstantPool().integers;
            return index < integers.size() && integers[index] == expected;
        }

        bool isIgnorableSentinelStoreSeparator(OpCode opcode)
        {
            switch (opcode)
            {
                case OpCode::POP:
                case OpCode::LINE:
                case OpCode::SOURCE_FILE:
                case OpCode::NOP:
                    return true;
                default:
                    return false;
            }
        }

        bool findConstLocalWrite(const bytecode::BytecodeProgram& program,
                                  size_t pushOffset,
                                  size_t endOffsetInclusive,
                                  int64_t expectedValue,
                                  size_t* outSlot,
                                  size_t* outNextOffset)
        {
            if (!isPushIntValue(program, program.getInstruction(pushOffset), expectedValue))
                return false;

            for (size_t ip = pushOffset + 1;
                 ip <= endOffsetInclusive && ip <= pushOffset + 4;
                 ++ip)
            {
                const auto& instr = program.getInstruction(ip);
                if (isIgnorableSentinelStoreSeparator(instr.opcode))
                    continue;

                size_t slot = 0;
                if (!localSlotOpcode(instr.opcode, instr, slot))
                    return false;

                if (outSlot)
                    *outSlot = slot;

                size_t next = ip + 1;
                while (next <= endOffsetInclusive &&
                       isIgnorableSentinelStoreSeparator(program.getInstruction(next).opcode))
                {
                    ++next;
                }
                if (outNextOffset)
                    *outNextOffset = next;
                return true;
            }
            return false;
        }

        bool isCallLikeOpcode(OpCode opcode)
        {
            switch (opcode)
            {
                case OpCode::CALL:
                case OpCode::CALL_FAST:
                case OpCode::CALL_STATIC:
                case OpCode::CALL_METHOD:
                case OpCode::CALL_METHOD_CACHED:
                case OpCode::CALL_METHOD_POLY_CACHED:
                case OpCode::LOAD_LOCAL_CALL_CACHED:
                case OpCode::LOAD_LOCAL_CALL_POLY_CACHED:
                    return true;
                default:
                    return false;
            }
        }

        bool isFieldReadOpcode(OpCode opcode)
        {
            switch (opcode)
            {
                case OpCode::GET_FIELD:
                case OpCode::GET_FIELD_CACHED:
                case OpCode::GET_FIELD_TYPED:
                case OpCode::INLINE_GET_FIELD:
                case OpCode::LOAD_GET_FIELD:
                case OpCode::LOAD_LOCAL_GET_FIELD_CACHED:
                    return true;
                default:
                    return false;
            }
        }

        bool isIgnorableConditionProducerSeparator(OpCode opcode)
        {
            switch (opcode)
            {
                case OpCode::LINE:
                case OpCode::SOURCE_FILE:
                case OpCode::NOP:
                    return true;
                default:
                    return false;
            }
        }

        bool priorConditionProducerIsFieldRead(const bytecode::BytecodeProgram& program,
                                                size_t ip,
                                                size_t startOffset)
        {
            while (ip > startOffset)
            {
                --ip;
                const auto opcode = program.getInstruction(ip).opcode;
                if (isIgnorableConditionProducerSeparator(opcode))
                    continue;
                return isFieldReadOpcode(opcode);
            }
            return false;
        }

        bool functionContainsLoopAndCall(
            const bytecode::BytecodeProgram& program,
            const bytecode::BytecodeProgram::FunctionMetadata* callee)
        {
            if (!callee || callee->returnType != "void")
                return false;

            bool hasLoopStart = false;
            bool hasJumpBack = false;
            bool hasCall = false;
            const size_t end = callee->startOffset + callee->instructionCount;
            for (size_t ip = callee->startOffset; ip < end; ++ip)
            {
                const auto& instr = program.getInstruction(ip);
                if (instr.opcode == OpCode::LOOP_START)
                    hasLoopStart = true;
                else if (instr.opcode == OpCode::JUMP_BACK)
                    hasJumpBack = true;
                else if (isCallLikeOpcode(instr.opcode))
                    hasCall = true;
            }

            return hasLoopStart && hasJumpBack && hasCall;
        }

        bool isUnsafeVoidLoopCall(
            const bytecode::BytecodeProgram& program,
            const bytecode::BytecodeProgram::Instruction& instr)
        {
            if (instr.numOperands() == 0)
                return false;

            if (instr.opcode == OpCode::CALL_FAST)
            {
                const auto* callee = program.getFunctionByIndex(
                    static_cast<size_t>(instr.inlineOperands[0]));
                return functionContainsLoopAndCall(program, callee);
            }

            if (instr.opcode == OpCode::CALL)
            {
                const auto nameIndex = static_cast<size_t>(instr.inlineOperands[0]);
                if (nameIndex >= program.getConstantPool().strings.size())
                    return false;
                const std::string& name = program.getConstantPool().getString(nameIndex);
                return functionContainsLoopAndCall(program, program.getFunction(name));
            }

            return false;
        }
    }

    uint64_t JitCompiler::getLoopConditionGuardHits()
    {
        return g_loopConditionGuardHits;
    }

    bool isPrimitiveOrVoidReturnType(const std::string& type)
    {
        return type == "void" || type == "int" || type == "float" || type == "bool";
    }

    bool hasUnsafeOrPopLoopShape(const bytecode::BytecodeProgram& program,
                                  size_t startOffset,
                                  size_t endOffsetInclusive)
    {
        bool hasLoopStart = false;
        bool hasJumpBack = false;
        size_t orPopCount = 0;

        for (size_t ip = startOffset; ip <= endOffsetInclusive; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            switch (instr.opcode)
            {
                case OpCode::LOOP_START: hasLoopStart = true; break;
                case OpCode::JUMP_BACK:  hasJumpBack = true;  break;
                case OpCode::JUMP_IF_TRUE_OR_POP:
                case OpCode::JUMP_IF_FALSE_OR_POP: ++orPopCount; break;
                default: break;
            }
        }

        return hasLoopStart && hasJumpBack && orPopCount >= 3;
    }

    bool hasUnsafeEarlyBoxedReturnLoopShape(const bytecode::BytecodeProgram& program,
                                             size_t startOffset,
                                             size_t endOffsetInclusive)
    {
        bool hasLoopStart = false;
        bool hasJumpBack = false;
        size_t returnValueCount = 0;

        for (size_t ip = startOffset; ip <= endOffsetInclusive; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            switch (instr.opcode)
            {
                case OpCode::LOOP_START: hasLoopStart = true; break;
                case OpCode::JUMP_BACK:  hasJumpBack = true;  break;
                case OpCode::RETURN_VALUE: ++returnValueCount; break;
                default: break;
            }
        }

        return hasLoopStart && hasJumpBack && returnValueCount >= 3;
    }

    bool hasArrayDeadStoreSentinelLoopShape(const bytecode::BytecodeProgram& program,
                                             size_t startOffset,
                                             size_t endOffsetInclusive)
    {
        bool hasLoopStart = false;
        bool hasJumpBack = false;
        bool hasArrayAccess = false;
        bool hasDeadStoreSentinel = false;

        for (size_t ip = startOffset; ip <= endOffsetInclusive; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            switch (instr.opcode)
            {
                case OpCode::LOOP_START:
                    hasLoopStart = true;
                    break;
                case OpCode::JUMP_BACK:
                    hasJumpBack = true;
                    break;
                default:
                    break;
            }

            if (isArrayAccessOpcode(instr.opcode))
                hasArrayAccess = true;

            size_t slot = 0;
            size_t nextOffset = 0;
            if (findConstLocalWrite(program, ip, endOffsetInclusive,
                                    0, &slot, &nextOffset))
            {
                size_t secondSlot = 0;
                size_t ignoredNext = 0;
                if (nextOffset <= endOffsetInclusive &&
                    findConstLocalWrite(program, nextOffset, endOffsetInclusive,
                                        -1, &secondSlot, &ignoredNext) &&
                    secondSlot == slot)
                {
                    hasDeadStoreSentinel = true;
                }
            }
        }

        return hasLoopStart && hasJumpBack && hasArrayAccess && hasDeadStoreSentinel;
    }

    bool hasGlobalArrayLoopShape(const bytecode::BytecodeProgram& program,
                                  size_t startOffset,
                                  size_t endOffsetInclusive)
    {
        bool hasLoopStart = false;
        bool hasJumpBack = false;
        bool hasArrayAccess = false;
        bool hasVariableAccess = false;

        for (size_t ip = startOffset; ip <= endOffsetInclusive; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            switch (instr.opcode)
            {
                case OpCode::LOOP_START:
                    hasLoopStart = true;
                    break;
                case OpCode::JUMP_BACK:
                    hasJumpBack = true;
                    break;
                default:
                    break;
            }

            if (isArrayAccessOpcode(instr.opcode))
                hasArrayAccess = true;
            if (isVariableAccessOpcode(instr.opcode))
                hasVariableAccess = true;
        }

        return hasLoopStart && hasJumpBack && hasArrayAccess && hasVariableAccess;
    }

    bool hasForwardConditionalCallRegion(const bytecode::BytecodeProgram& program,
                                          size_t startOffset,
                                          size_t endOffsetInclusive,
                                          OpCode* outOpcode)
    {
        for (size_t ip = startOffset; ip <= endOffsetInclusive; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            switch (instr.opcode)
            {
                case OpCode::JUMP_IF_FALSE:
                case OpCode::JUMP_IF_TRUE:
                {
                    if (instr.numOperands() == 0) break;
                    const size_t target = static_cast<size_t>(instr.inlineOperands[0]);
                    if (target <= ip + 1 || target > endOffsetInclusive + 1)
                        break;
                    // `for` lowers its loop condition as:
                    //   JUMP_IF_FALSE exit; JUMP body; increment; JUMP_BACK;
                    // Scanning that false-edge would classify the whole loop
                    // body as a conditional region and block every hot loop
                    // with a call in it. The unsafe MYT-302 shape is a normal
                    // in-body branch, not the synthetic loop-condition jump.
                    if (ip + 1 <= endOffsetInclusive &&
                        program.getInstruction(ip + 1).opcode == OpCode::JUMP)
                    {
                        // Counter so a future refactor of for-loop lowering
                        // that breaks this pattern-match surfaces in tests
                        // (the guard would silently become a no-op otherwise).
                        ++g_loopConditionGuardHits;
                        break;
                    }
                    bool hasCall = false;
                    size_t callCount = 0;
                    bool hasUnsafeVoidCall = false;
                    OpCode firstCallOpcode = OpCode::CALL_METHOD;
                    for (size_t bodyIp = ip + 1;
                         bodyIp < target && bodyIp <= endOffsetInclusive;
                         ++bodyIp)
                    {
                        const auto& bodyInstr = program.getInstruction(bodyIp);
                        if (isCallLikeOpcode(bodyInstr.opcode))
                        {
                            if (!hasCall)
                                firstCallOpcode = bodyInstr.opcode;
                            hasCall = true;
                            ++callCount;
                            if (isUnsafeVoidLoopCall(program, bodyInstr))
                                hasUnsafeVoidCall = true;
                        }
                    }

                    bool fieldProducer =
                        priorConditionProducerIsFieldRead(program, ip, startOffset);
                    bool reject = hasCall &&
                        (fieldProducer || callCount >= 3 || hasUnsafeVoidCall);
                    if (reject)
                    {
                        if (outOpcode)
                            *outOpcode = firstCallOpcode;
                        return true;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        return false;
    }
}
