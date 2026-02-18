#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    // ===== Shared helpers for call-type opcodes =====

    void emitBoxCallArgs(JitEmissionState& s, size_t argCount, size_t destStartSlot)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        for (size_t i = 0; i < argCount; ++i)
        {
            int stackIdx = s.stackDepth - static_cast<int>(argCount) + static_cast<int>(i);
            SlotType argType = (stackIdx >= 0 && stackIdx < static_cast<int>(s.slotTypes.size()))
                             ? s.slotTypes[stackIdx] : SlotType::INT;
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)
                                  + static_cast<int32_t>((i + destStartSlot) * valueSize)));

            if (s.usesBoxedTypes && isBoxedSlotType(argType))
            {
                Gp srcAddr = cc.new_gp64();
                cc.lea(srcAddr, Mem(s.boxedBase, static_cast<int32_t>(stackIdx * valueSize)));
                InvokeNode* cpInv;
                cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                          FuncSignature::build<void, value::Value*, const value::Value*>());
                cpInv->set_arg(0, destAddr);
                cpInv->set_arg(1, srcAddr);
            }
            else
            {
                emitBox(s, destAddr, stackIdx, argType);
            }
        }
    }

    void emitPopAndDestroyArgs(JitEmissionState& s, size_t argCount)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        for (size_t i = 0; i < argCount; ++i)
        {
            SlotType at = popType(s);
            if (s.usesBoxedTypes && isBoxedSlotType(at))
            {
                int idx = s.stackDepth - static_cast<int>(argCount) + static_cast<int>(i);
                Gp addr = cc.new_gp64();
                cc.lea(addr, Mem(s.boxedBase, static_cast<int32_t>(idx * valueSize)));
                InvokeNode* dInv;
                cc.invoke(Out(dInv), reinterpret_cast<uint64_t>(jit_value_destroy),
                          FuncSignature::build<void, value::Value*>());
                dInv->set_arg(0, addr);
            }
        }
        s.stackDepth -= static_cast<int>(argCount);
    }

    // ===== Object, String, Array, Method, Type Operations =====

    bool emitObjectOps(JitEmissionState& s,
                       const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        // Delegate array opcodes to JitCompiler_Arrays.cpp
        if (emitArrayOps(s, instr)) return true;

        switch (instr.opcode)
        {
            case OpCode::PUSH_STRING:
            {
                uint32_t constIndex = static_cast<uint32_t>(instr.operands[0]);
                Gp dest = cc.new_gp64();
                cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
                Gp pPtr = cc.new_gp64();
                cc.mov(pPtr, s.progPtr);
                Gp idx = cc.new_gp64();
                cc.mov(idx, static_cast<int64_t>(constIndex));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_push_string),
                          FuncSignature::build<void, value::Value*,
                              const vm::bytecode::BytecodeProgram*, uint32_t>());
                inv->set_arg(0, dest);
                inv->set_arg(1, pPtr);
                inv->set_arg(2, idx);
                s.slotTypes.push_back(SlotType::STRING);
                s.stackDepth++;
                return true;
            }

            case OpCode::GET_FIELD:
            {
                uint32_t fieldNameIndex = static_cast<uint32_t>(instr.operands[0]);
                Gp objAddr = cc.new_gp64();
                cc.lea(objAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
                Gp dest = cc.new_gp64();
                cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
                Gp ipReg = cc.new_gp64();
                cc.mov(ipReg, static_cast<int64_t>(s.currentIP));
                Gp idx = cc.new_gp64();
                cc.mov(idx, static_cast<int64_t>(fieldNameIndex));
                Gp flagsReg = cc.new_gp64();
                cc.mov(flagsReg, static_cast<int64_t>(instr.flags));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_get_field_ic),
                          FuncSignature::build<void, value::Value*, const value::Value*,
                              JitContext*, size_t, uint32_t, uint8_t>());
                inv->set_arg(0, dest);
                inv->set_arg(1, objAddr);
                inv->set_arg(2, s.ctxPtr);
                inv->set_arg(3, ipReg);
                inv->set_arg(4, idx);
                inv->set_arg(5, flagsReg);
                popType(s);
                s.slotTypes.push_back(SlotType::BOXED);
                return true;
            }

            case OpCode::SET_FIELD:
            {
                uint32_t fieldNameIndex = static_cast<uint32_t>(instr.operands[0]);
                SlotType valType = popType(s);
                popType(s);

                Gp newValAddr = cc.new_gp64();
                if (!isBoxedSlotType(valType))
                {
                    cc.lea(newValAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)));
                    emitBox(s, newValAddr, s.stackDepth - 1, valType);
                }
                else
                {
                    cc.lea(newValAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
                }

                Gp objAddr = cc.new_gp64();
                cc.lea(objAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));
                Gp dest = cc.new_gp64();
                cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));
                Gp ipReg = cc.new_gp64();
                cc.mov(ipReg, static_cast<int64_t>(s.currentIP));
                Gp idx = cc.new_gp64();
                cc.mov(idx, static_cast<int64_t>(fieldNameIndex));
                Gp flagsReg = cc.new_gp64();
                cc.mov(flagsReg, static_cast<int64_t>(instr.flags));

                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_field_ic),
                          FuncSignature::build<void, value::Value*, const value::Value*,
                              const value::Value*,
                              JitContext*, size_t, uint32_t, uint8_t>());
                inv->set_arg(0, dest);
                inv->set_arg(1, objAddr);
                inv->set_arg(2, newValAddr);
                inv->set_arg(3, s.ctxPtr);
                inv->set_arg(4, ipReg);
                inv->set_arg(5, idx);
                inv->set_arg(6, flagsReg);

                if (isBoxedSlotType(valType))
                {
                    Gp vAddr = cc.new_gp64();
                    cc.lea(vAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
                    InvokeNode* dInv;
                    cc.invoke(Out(dInv), reinterpret_cast<uint64_t>(jit_value_destroy),
                              FuncSignature::build<void, value::Value*>());
                    dInv->set_arg(0, vAddr);
                }

                s.stackDepth--;
                s.slotTypes.push_back(SlotType::BOXED);
                return true;
            }

            case OpCode::CALL_STATIC:
            {
                uint32_t nameIndex = static_cast<uint32_t>(instr.operands[0]);
                size_t argCount = instr.operands[1];

                if (argCount > JitContext::MAX_CALL_ARGS)
                {
                    s.compileFailed = true;
                    return true;
                }

                emitBoxCallArgs(s, argCount);
                emitPopAndDestroyArgs(s, argCount);

                Gp niReg = cc.new_gp64();
                cc.mov(niReg, static_cast<int64_t>(nameIndex));
                Gp acReg = cc.new_gp64();
                cc.mov(acReg, static_cast<int64_t>(argCount));

                InvokeNode* callInv;
                cc.invoke(Out(callInv), reinterpret_cast<uint64_t>(jit_call_static),
                          FuncSignature::build<void, JitContext*, uint32_t, size_t>());
                callInv->set_arg(0, s.ctxPtr);
                callInv->set_arg(1, niReg);
                callInv->set_arg(2, acReg);

                {
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
                return true;
            }

            case OpCode::CALL_METHOD:
            {
                uint32_t methodNameIndex = static_cast<uint32_t>(instr.operands[0]);
                size_t argCount = instr.operands[1];

                if (argCount + 1 > JitContext::MAX_CALL_ARGS)
                {
                    s.compileFailed = true;
                    return true;
                }

                // Box object into callArgs[0]
                {
                    int objIdx = s.stackDepth - static_cast<int>(argCount) - 1;
                    Gp destAddr = cc.new_gp64();
                    cc.lea(destAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)));
                    Gp srcAddr = cc.new_gp64();
                    cc.lea(srcAddr, Mem(s.boxedBase, static_cast<int32_t>(objIdx * valueSize)));
                    InvokeNode* cpInv;
                    cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                              FuncSignature::build<void, value::Value*, const value::Value*>());
                    cpInv->set_arg(0, destAddr);
                    cpInv->set_arg(1, srcAddr);
                }

                // Box args into callArgs[1..argCount]
                emitBoxCallArgs(s, argCount, 1);

                // Pop args + object, destroy boxed slots
                for (size_t i = 0; i < argCount; ++i)
                {
                    SlotType at = popType(s);
                    if (isBoxedSlotType(at))
                    {
                        int idx = s.stackDepth - static_cast<int>(argCount) + static_cast<int>(i) - 1;
                        Gp addr = cc.new_gp64();
                        cc.lea(addr, Mem(s.boxedBase, static_cast<int32_t>((idx + 1) * valueSize)));
                        InvokeNode* dInv;
                        cc.invoke(Out(dInv), reinterpret_cast<uint64_t>(jit_value_destroy),
                                  FuncSignature::build<void, value::Value*>());
                        dInv->set_arg(0, addr);
                    }
                }
                popType(s); // pop object type
                {
                    int objIdx = s.stackDepth - static_cast<int>(argCount) - 1;
                    Gp addr = cc.new_gp64();
                    cc.lea(addr, Mem(s.boxedBase, static_cast<int32_t>(objIdx * valueSize)));
                    InvokeNode* dInv;
                    cc.invoke(Out(dInv), reinterpret_cast<uint64_t>(jit_value_destroy),
                              FuncSignature::build<void, value::Value*>());
                    dInv->set_arg(0, addr);
                }
                s.stackDepth -= static_cast<int>(argCount) + 1;

                Gp miReg = cc.new_gp64();
                cc.mov(miReg, static_cast<int64_t>(methodNameIndex));
                Gp acReg = cc.new_gp64();
                cc.mov(acReg, static_cast<int64_t>(argCount));

                InvokeNode* callInv;
                cc.invoke(Out(callInv), reinterpret_cast<uint64_t>(jit_call_method),
                          FuncSignature::build<void, JitContext*, uint32_t, size_t>());
                callInv->set_arg(0, s.ctxPtr);
                callInv->set_arg(1, miReg);
                callInv->set_arg(2, acReg);

                {
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
                return true;
            }

            case OpCode::INSTANCEOF:
            {
                uint32_t typeIndex = static_cast<uint32_t>(instr.operands[0]);
                SlotType vType = popType(s);

                Gp valAddr = cc.new_gp64();
                if (!isBoxedSlotType(vType))
                {
                    cc.lea(valAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)));
                    emitBox(s, valAddr, s.stackDepth - 1, vType);
                }
                else
                {
                    cc.lea(valAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
                }

                Gp pPtr = cc.new_gp64();
                cc.mov(pPtr, s.progPtr);
                Gp idx = cc.new_gp64();
                cc.mov(idx, static_cast<int64_t>(typeIndex));

                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_instanceof),
                          FuncSignature::build<int64_t, const value::Value*,
                              const vm::bytecode::BytecodeProgram*, uint32_t>());
                inv->set_arg(0, valAddr);
                inv->set_arg(1, pPtr);
                inv->set_arg(2, idx);
                Gp result = cc.new_gp64();
                inv->set_ret(0, result);

                if (isBoxedSlotType(vType))
                {
                    Gp dAddr = cc.new_gp64();
                    cc.lea(dAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
                    InvokeNode* dInv;
                    cc.invoke(Out(dInv), reinterpret_cast<uint64_t>(jit_value_destroy),
                              FuncSignature::build<void, value::Value*>());
                    dInv->set_arg(0, dAddr);
                }

                cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), result);
                s.slotTypes.push_back(SlotType::BOOL);
                return true;
            }

            case OpCode::CAST:
            {
                uint32_t typeIndex = static_cast<uint32_t>(instr.operands[0]);
                SlotType srcType = popType(s);

                Gp srcAddr = cc.new_gp64();
                if (!isBoxedSlotType(srcType))
                {
                    cc.lea(srcAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)));
                    emitBox(s, srcAddr, s.stackDepth - 1, srcType);
                }
                else
                {
                    cc.lea(srcAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
                }

                Gp dest = cc.new_gp64();
                cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
                Gp pPtr = cc.new_gp64();
                cc.mov(pPtr, s.progPtr);
                Gp idx = cc.new_gp64();
                cc.mov(idx, static_cast<int64_t>(typeIndex));

                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_cast),
                          FuncSignature::build<void, value::Value*, const value::Value*,
                              const vm::bytecode::BytecodeProgram*, uint32_t>());
                inv->set_arg(0, dest);
                inv->set_arg(1, srcAddr);
                inv->set_arg(2, pPtr);
                inv->set_arg(3, idx);

                s.slotTypes.push_back(SlotType::BOXED);
                return true;
            }

            case OpCode::NEW_OBJECT:
            {
                uint32_t classIndex = static_cast<uint32_t>(instr.operands[0]);
                size_t argCount = instr.operands[1];

                if (argCount > JitContext::MAX_CALL_ARGS)
                {
                    s.compileFailed = true;
                    return true;
                }

                emitBoxCallArgs(s, argCount);
                emitPopAndDestroyArgs(s, argCount);

                Gp dest = cc.new_gp64();
                cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
                Gp ciReg = cc.new_gp64();
                cc.mov(ciReg, static_cast<int64_t>(classIndex));
                Gp acReg = cc.new_gp64();
                cc.mov(acReg, static_cast<int64_t>(argCount));

                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_new_object),
                          FuncSignature::build<void, value::Value*, JitContext*,
                              uint32_t, size_t>());
                inv->set_arg(0, dest);
                inv->set_arg(1, s.ctxPtr);
                inv->set_arg(2, ciReg);
                inv->set_arg(3, acReg);

                s.slotTypes.push_back(SlotType::OBJECT);
                s.stackDepth++;
                return true;
            }

            case OpCode::NEW_VALUE_OBJECT:
            {
                // Value object construction uses the same mechanism as regular objects.
                // The constructor needs an ObjectInstance for 'this'.
                // After the constructor, OBJECT_TO_VALUE converts to ValueObject.
                uint32_t classIndex = static_cast<uint32_t>(instr.operands[0]);
                size_t argCount = instr.operands[1];

                if (argCount > JitContext::MAX_CALL_ARGS)
                {
                    s.compileFailed = true;
                    return true;
                }

                emitBoxCallArgs(s, argCount);
                emitPopAndDestroyArgs(s, argCount);

                Gp dest = cc.new_gp64();
                cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
                Gp ciReg = cc.new_gp64();
                cc.mov(ciReg, static_cast<int64_t>(classIndex));
                Gp acReg = cc.new_gp64();
                cc.mov(acReg, static_cast<int64_t>(argCount));

                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_new_object),
                          FuncSignature::build<void, value::Value*, JitContext*,
                              uint32_t, size_t>());
                inv->set_arg(0, dest);
                inv->set_arg(1, s.ctxPtr);
                inv->set_arg(2, ciReg);
                inv->set_arg(3, acReg);

                s.slotTypes.push_back(SlotType::OBJECT);
                s.stackDepth++;
                return true;
            }

            case OpCode::OBJECT_TO_VALUE:
            {
                // Convert ObjectInstance on stack top to lightweight ValueObject
                Gp addr = cc.new_gp64();
                cc.lea(addr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));

                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_object_to_value),
                          FuncSignature::build<void, value::Value*>());
                inv->set_arg(0, addr);

                popType(s);
                s.slotTypes.push_back(SlotType::VALUE_OBJECT);
                return true;
            }

            default:
                return false;
        }
    }
}
