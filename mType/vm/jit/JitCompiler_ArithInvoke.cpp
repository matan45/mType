#include "JitCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    // Defined in JitCompiler_ArithInvokeInt.cpp.
    bool emitInvokeIntBinary(JitEmissionState& s, OpCode op);
    bool emitInvokeIntUnary(JitEmissionState& s, OpCode op);
    bool emitInvokeIntGetValue(JitEmissionState& s);

    // Defined in JitCompiler_ArithInvokeFloat.cpp.
    bool emitInvokeFloatBinary(JitEmissionState& s, OpCode op);
    bool emitInvokeFloatUnary(JitEmissionState& s, OpCode op);
    bool emitInvokeFloatGetValue(JitEmissionState& s);

    // Defined in JitCompiler_ArithInvokeBool.cpp.
    bool emitInvokeBoolGetValue(JitEmissionState& s);
    bool emitInvokeBoolBinary(JitEmissionState& s, OpCode op);
    bool emitInvokeBoolNot(JitEmissionState& s);

    namespace
    {
        // INVOKE_STRING_LENGTH/IS_EMPTY: receiver lives on boxedBase (boxed-
        // mode trigger fires for these opcodes); call helper, push int/bool.
        bool emitInvokeStringUnary(JitEmissionState& s, OpCode op)
        {
            auto& cc = s.cc;
            constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
            SlotType rType = popType(s);
            Gp recvAddr = cc.new_gp64();
            cc.lea(recvAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
            InvokeNode* inv;
            uint64_t fn = (op == OpCode::INVOKE_STRING_LENGTH)
                ? reinterpret_cast<uint64_t>(jit_invoke_string_length)
                : reinterpret_cast<uint64_t>(jit_invoke_string_is_empty);
            cc.invoke(Out(inv), fn,
                      FuncSignature::build<int64_t, const value::Value*>());
            inv->set_arg(0, recvAddr);
            Gp ret = cc.new_gp64();
            inv->set_ret(0, ret);
            // Destroy the consumed boxed receiver and write the raw result
            // into stackBase[stackDepth-1].
            InvokeNode* dest;
            cc.invoke(Out(dest), reinterpret_cast<uint64_t>(jit_value_destroy),
                      FuncSignature::build<void, value::Value*>());
            dest->set_arg(0, recvAddr);
            cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), ret);
            s.slotTypes.push_back(op == OpCode::INVOKE_STRING_LENGTH
                                    ? SlotType::INT : SlotType::BOOL);
            (void)rType;
            return true;
        }

        bool emitInvokeStringEquals(JitEmissionState& s)
        {
            auto& cc = s.cc;
            constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
            s.stackDepth--;
            SlotType rType = popType(s);
            SlotType lType = popType(s);
            Gp argAddr = cc.new_gp64();
            cc.lea(argAddr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
            Gp recvAddr = cc.new_gp64();
            cc.lea(recvAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_invoke_string_equals),
                      FuncSignature::build<int64_t, const value::Value*, const value::Value*>());
            inv->set_arg(0, recvAddr);
            inv->set_arg(1, argAddr);
            Gp ret = cc.new_gp64();
            inv->set_ret(0, ret);
            InvokeNode* dArg;
            cc.invoke(Out(dArg), reinterpret_cast<uint64_t>(jit_value_destroy),
                      FuncSignature::build<void, value::Value*>());
            dArg->set_arg(0, argAddr);
            InvokeNode* dRecv;
            cc.invoke(Out(dRecv), reinterpret_cast<uint64_t>(jit_value_destroy),
                      FuncSignature::build<void, value::Value*>());
            dRecv->set_arg(0, recvAddr);
            cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), ret);
            s.slotTypes.push_back(SlotType::BOOL);
            (void)rType; (void)lType;
            return true;
        }

        // INVOKE_STRING_CONCAT: jit_invoke_string_concat reads receiver+arg,
        // destroys *dest in place, writes the new boxed (interned-when-able)
        // result. dest = receiver slot.
        bool emitInvokeStringConcat(JitEmissionState& s)
        {
            auto& cc = s.cc;
            constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
            s.stackDepth--;
            SlotType rType = popType(s);
            SlotType lType = popType(s);
            Gp argAddr = cc.new_gp64();
            cc.lea(argAddr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
            Gp recvAddr = cc.new_gp64();
            cc.lea(recvAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_invoke_string_concat),
                      FuncSignature::build<void, value::Value*, const value::Value*, const value::Value*>());
            inv->set_arg(0, recvAddr);  // dest
            inv->set_arg(1, recvAddr);  // receiver (read before destroy)
            inv->set_arg(2, argAddr);
            InvokeNode* dArg;
            cc.invoke(Out(dArg), reinterpret_cast<uint64_t>(jit_value_destroy),
                      FuncSignature::build<void, value::Value*>());
            dArg->set_arg(0, argAddr);
            s.slotTypes.push_back(SlotType::BOXED);
            (void)rType; (void)lType;
            return true;
        }
    }

    bool emitInvokePrimitiveOps(JitEmissionState& s,
                                 const bytecode::BytecodeProgram::Instruction& instr)
    {
        // MYT-211: INVOKE_INT_*/INVOKE_FLOAT_* emitters read stackBase memory
        // directly and aren't hint-aware. Flush at the dispatch level so any
        // prior cached value is materialized to memory before they run.
        flushAllHints(s);
        switch (instr.opcode)
        {
            case OpCode::INVOKE_INT_ADD: case OpCode::INVOKE_INT_SUB:
            case OpCode::INVOKE_INT_MUL: case OpCode::INVOKE_INT_DIV:
            case OpCode::INVOKE_INT_MOD:
            case OpCode::INVOKE_INT_EQUALS: case OpCode::INVOKE_INT_COMPARE:
            case OpCode::INVOKE_INT_LESS_THAN: case OpCode::INVOKE_INT_LESS_EQUAL:
            case OpCode::INVOKE_INT_GREATER_THAN: case OpCode::INVOKE_INT_GREATER_EQUAL:
                return emitInvokeIntBinary(s, instr.opcode);

            case OpCode::INVOKE_INT_NEG: case OpCode::INVOKE_INT_ABS:
                return emitInvokeIntUnary(s, instr.opcode);

            case OpCode::INVOKE_INT_GET_VALUE:
                return emitInvokeIntGetValue(s);

            case OpCode::INVOKE_FLOAT_ADD: case OpCode::INVOKE_FLOAT_SUB:
            case OpCode::INVOKE_FLOAT_MUL: case OpCode::INVOKE_FLOAT_DIV:
            case OpCode::INVOKE_FLOAT_EQUALS: case OpCode::INVOKE_FLOAT_COMPARE:
            case OpCode::INVOKE_FLOAT_LESS_THAN: case OpCode::INVOKE_FLOAT_LESS_EQUAL:
            case OpCode::INVOKE_FLOAT_GREATER_THAN: case OpCode::INVOKE_FLOAT_GREATER_EQUAL:
                return emitInvokeFloatBinary(s, instr.opcode);

            case OpCode::INVOKE_FLOAT_NEG: case OpCode::INVOKE_FLOAT_ABS:
                return emitInvokeFloatUnary(s, instr.opcode);

            case OpCode::INVOKE_FLOAT_GET_VALUE:
                return emitInvokeFloatGetValue(s);

            case OpCode::INVOKE_BOOL_GET_VALUE:
                return emitInvokeBoolGetValue(s);

            case OpCode::INVOKE_BOOL_AND:
            case OpCode::INVOKE_BOOL_OR:
            case OpCode::INVOKE_BOOL_XOR:
            case OpCode::INVOKE_BOOL_EQUALS:
                return emitInvokeBoolBinary(s, instr.opcode);
            case OpCode::INVOKE_BOOL_NOT:
                return emitInvokeBoolNot(s);

            case OpCode::INVOKE_STRING_LENGTH:
            case OpCode::INVOKE_STRING_IS_EMPTY:
                return emitInvokeStringUnary(s, instr.opcode);
            case OpCode::INVOKE_STRING_EQUALS:
                return emitInvokeStringEquals(s);
            case OpCode::INVOKE_STRING_CONCAT:
                return emitInvokeStringConcat(s);

            default: return false;
        }
    }
}
