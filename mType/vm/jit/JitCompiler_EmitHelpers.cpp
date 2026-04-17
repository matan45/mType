#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "JitCodeCache.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    void emitValueDestroy(JitEmissionState& s, int slotOffset)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        Gp addr = cc.new_gp64();
        cc.lea(addr, Mem(s.boxedBase, static_cast<int32_t>(slotOffset * valueSize)));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_destroy),
                  FuncSignature::build<void, value::Value*>());
        inv->set_arg(0, addr);
    }

    void emitReturnValueCopyBoxed(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        Gp retAddr = cc.new_gp64();
        cc.lea(retAddr, Mem(s.ctxPtr, offsetof(JitContext, returnValue)));
        Gp destAddr = cc.new_gp64();
        cc.lea(destAddr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        InvokeNode* cpInv;
        cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                  FuncSignature::build<void, value::Value*, const value::Value*>());
        cpInv->set_arg(0, destAddr);
        cpInv->set_arg(1, retAddr);
        s.slotTypes.push_back(SlotType::BOXED);
        s.stackDepth++;
    }

    Gp emitGetBoxedValueAddr(JitEmissionState& s, int stackIdx, SlotType valType)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        Gp addr = cc.new_gp64();
        if (!isBoxedSlotType(valType))
        {
            cc.lea(addr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)));
            emitBox(s, addr, stackIdx, valType);
        }
        else
        {
            cc.lea(addr, Mem(s.boxedBase, static_cast<int32_t>(stackIdx * valueSize)));
        }
        return addr;
    }

    bool scanOpcodesForBoxedTypes(const bytecode::BytecodeProgram& program,
                                  size_t startOffset, size_t endOffset)
    {
        for (size_t ip = startOffset; ip < endOffset; ++ip)
        {
            const auto& si = program.getInstruction(ip);
            switch (si.opcode)
            {
                case OpCode::PUSH_STRING: case OpCode::GET_FIELD:
                case OpCode::SET_FIELD:   case OpCode::INLINE_SET_FIELD:
                // MYT-152: LOAD_VAR / STORE_VAR produce / consume boxed Values
                // (global or field lookups have no compile-time primitive
                // type), so the enclosing loop must emit in boxed mode.
                case OpCode::LOAD_VAR:    case OpCode::STORE_VAR:
                case OpCode::NEW_OBJECT:
                case OpCode::NEW_VALUE_OBJECT: case OpCode::OBJECT_TO_VALUE:
                case OpCode::CALL_METHOD: case OpCode::CALL_STATIC:
                case OpCode::NEW_ARRAY:   case OpCode::NEW_ARRAY_MULTI:
                case OpCode::ARRAY_GET:
                case OpCode::ARRAY_SET:   case OpCode::ARRAY_LENGTH:
                case OpCode::ARRAY_GET_INT_LOCAL: case OpCode::ARRAY_SET_INT_LOCAL:
                case OpCode::ARRAY_LENGTH_LOCAL:
                case OpCode::INSTANCEOF:  case OpCode::INSTANCEOF_TYPEPARAM:
                case OpCode::CAST:
                // MYT-147: iterator opcodes receive / produce boxed
                // ObjectInstance values (ArrayIteratorHelper,
                // HashMapKeyIterator, LinkedListIterator, ...) on the operand
                // stack, so the enclosing function must run in boxed-types
                // mode. ITERATOR_HAS_NEXT pushes bool; ITERATOR_CLOSE pushes
                // nothing - but they're stack-adjacent to boxed iterator slots
                // and are safe to include in the boxed-mode trigger set.
                case OpCode::GET_ITERATOR:      case OpCode::ITERATOR_HAS_NEXT:
                case OpCode::ITERATOR_NEXT:     case OpCode::ITERATOR_CLOSE:
                // Specialized primitive-method opcodes receive boxed Int / Float
                // objects on the operand stack, so the enclosing function must
                // be emitted in boxed-types mode.
                case OpCode::INVOKE_INT_ADD: case OpCode::INVOKE_INT_SUB:
                case OpCode::INVOKE_INT_MUL: case OpCode::INVOKE_INT_DIV:
                case OpCode::INVOKE_INT_MOD: case OpCode::INVOKE_INT_NEG:
                case OpCode::INVOKE_INT_ABS: case OpCode::INVOKE_INT_EQUALS:
                case OpCode::INVOKE_INT_COMPARE:
                case OpCode::INVOKE_FLOAT_ADD: case OpCode::INVOKE_FLOAT_SUB:
                case OpCode::INVOKE_FLOAT_MUL: case OpCode::INVOKE_FLOAT_DIV:
                case OpCode::INVOKE_FLOAT_NEG: case OpCode::INVOKE_FLOAT_ABS:
                case OpCode::INVOKE_FLOAT_EQUALS: case OpCode::INVOKE_FLOAT_COMPARE:
                    return true;
                default: break;
            }
        }
        for (size_t ip = startOffset; ip < endOffset; ++ip)
        {
            const auto& si = program.getInstruction(ip);
            if (si.opcode == OpCode::CALL && si.operands.size() >= 2)
            {
                uint32_t fnIdx = static_cast<uint32_t>(si.operands[0]);
                if (fnIdx >= program.getConstantPool().strings.size())
                    continue;
                const std::string& fn = program.getConstantPool().getString(fnIdx);
                const auto* cm = program.getFunction(fn);
                if (cm && cm->returnType != "int" && cm->returnType != "float" &&
                    cm->returnType != "bool" && cm->returnType != "void")
                {
                    return true;
                }
            }
        }
        return false;
    }

    std::unordered_map<size_t, Label> createJumpLabels(
        Compiler& cc, const bytecode::BytecodeProgram& program,
        size_t startOffset, size_t endOffset,
        size_t rangeStart, size_t rangeEnd)
    {
        std::unordered_map<size_t, Label> labels;
        for (size_t ip = startOffset; ip < endOffset; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            if (instr.opcode == OpCode::JUMP || instr.opcode == OpCode::JUMP_IF_FALSE ||
                instr.opcode == OpCode::JUMP_IF_TRUE ||
                instr.opcode == OpCode::JUMP_IF_FALSE_OR_POP ||
                instr.opcode == OpCode::JUMP_IF_TRUE_OR_POP ||
                instr.opcode == OpCode::JUMP_BACK)
            {
                if (!instr.operands.empty())
                {
                    size_t target = instr.operands[0];
                    if (target >= rangeStart && target <= rangeEnd &&
                        labels.find(target) == labels.end())
                    {
                        labels[target] = cc.new_label();
                    }
                }
            }
        }
        return labels;
    }

    std::unordered_set<size_t> collectBackEdgeTargets(
        const bytecode::BytecodeProgram& program,
        size_t startOffset, size_t endOffset)
    {
        std::unordered_set<size_t> targets;
        for (size_t ip = startOffset; ip < endOffset; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            if (instr.opcode == OpCode::JUMP_BACK && !instr.operands.empty())
            {
                targets.insert(instr.operands[0]);
            }
        }
        return targets;
    }

    void emitMemoryInit(Compiler& cc,
                        Gp localsBase, size_t localCount,
                        Gp boxedBase, size_t boxedCount)
    {
        Gp initBase = cc.new_gp64();
        cc.mov(initBase, localsBase);
        Gp initCount = cc.new_gp64();
        cc.mov(initCount, static_cast<int64_t>(localCount));
        InvokeNode* initLocals;
        cc.invoke(Out(initLocals), reinterpret_cast<uint64_t>(jit_locals_init),
                  FuncSignature::build<void, value::Value*, size_t>());
        initLocals->set_arg(0, initBase);
        initLocals->set_arg(1, initCount);

        Gp bsBase = cc.new_gp64();
        cc.mov(bsBase, boxedBase);
        Gp bsCount = cc.new_gp64();
        cc.mov(bsCount, static_cast<int64_t>(boxedCount));
        InvokeNode* initBoxed;
        cc.invoke(Out(initBoxed), reinterpret_cast<uint64_t>(jit_locals_init),
                  FuncSignature::build<void, value::Value*, size_t>());
        initBoxed->set_arg(0, bsBase);
        initBoxed->set_arg(1, bsCount);
    }

    bool finalizeAndStore(Compiler& cc, CodeHolder& code,
                          JitCodeCache& codeCache, const std::string& key,
                          size_t& compileCount, size_t& bailoutCount)
    {
        Error err = cc.finalize();
        if (err != Error::kOk)
        {
            bailoutCount++;
            return false;
        }

        JitFunction fn = nullptr;
        err = codeCache.getRuntime().add(&fn, &code);
        if (err != Error::kOk)
        {
            bailoutCount++;
            return false;
        }

        codeCache.store(key, fn);
        compileCount++;
        return true;
    }
}
