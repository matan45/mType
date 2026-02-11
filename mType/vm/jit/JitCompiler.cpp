#include "JitCompiler.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>
#include <cstring>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    JitCompiler::JitCompiler() {}

    static std::string getKnownNativeReturnType(const std::string& name)
    {
        if (name == "print" || name == "println") return "void";
        return "";
    }

    const std::unordered_set<uint8_t>& JitCompiler::getSupportedOpcodes()
    {
        static const std::unordered_set<uint8_t> supported = {
            static_cast<uint8_t>(OpCode::PUSH_INT),
            static_cast<uint8_t>(OpCode::PUSH_FLOAT),
            static_cast<uint8_t>(OpCode::PUSH_BOOL),
            static_cast<uint8_t>(OpCode::PUSH_NULL),
            static_cast<uint8_t>(OpCode::POP),
            static_cast<uint8_t>(OpCode::DUP),
            static_cast<uint8_t>(OpCode::SWAP),

            static_cast<uint8_t>(OpCode::ADD_INT),
            static_cast<uint8_t>(OpCode::SUB_INT),
            static_cast<uint8_t>(OpCode::MUL_INT),
            static_cast<uint8_t>(OpCode::DIV_INT),
            static_cast<uint8_t>(OpCode::NEG),
            static_cast<uint8_t>(OpCode::INC),
            static_cast<uint8_t>(OpCode::DEC),

            static_cast<uint8_t>(OpCode::ADD_FLOAT),
            static_cast<uint8_t>(OpCode::SUB_FLOAT),
            static_cast<uint8_t>(OpCode::MUL_FLOAT),
            static_cast<uint8_t>(OpCode::DIV_FLOAT),

            static_cast<uint8_t>(OpCode::ADD),
            static_cast<uint8_t>(OpCode::SUB),
            static_cast<uint8_t>(OpCode::MUL),
            static_cast<uint8_t>(OpCode::DIV),
            static_cast<uint8_t>(OpCode::MOD),

            static_cast<uint8_t>(OpCode::EQ),
            static_cast<uint8_t>(OpCode::NE),
            static_cast<uint8_t>(OpCode::LT),
            static_cast<uint8_t>(OpCode::GT),
            static_cast<uint8_t>(OpCode::LE),
            static_cast<uint8_t>(OpCode::GE),
            static_cast<uint8_t>(OpCode::EQ_INT),
            static_cast<uint8_t>(OpCode::NE_INT),
            static_cast<uint8_t>(OpCode::LT_INT),
            static_cast<uint8_t>(OpCode::GT_INT),

            static_cast<uint8_t>(OpCode::NOT),
            static_cast<uint8_t>(OpCode::AND),
            static_cast<uint8_t>(OpCode::OR),

            static_cast<uint8_t>(OpCode::LOAD_LOCAL),
            static_cast<uint8_t>(OpCode::STORE_LOCAL),

            static_cast<uint8_t>(OpCode::JUMP),
            static_cast<uint8_t>(OpCode::JUMP_IF_FALSE),
            static_cast<uint8_t>(OpCode::JUMP_IF_TRUE),
            static_cast<uint8_t>(OpCode::JUMP_BACK),
            static_cast<uint8_t>(OpCode::RETURN),
            static_cast<uint8_t>(OpCode::RETURN_VALUE),

            static_cast<uint8_t>(OpCode::CALL),

            static_cast<uint8_t>(OpCode::LINE),
            static_cast<uint8_t>(OpCode::SOURCE_FILE),
            static_cast<uint8_t>(OpCode::NOP),
            static_cast<uint8_t>(OpCode::LOOP_START),
            static_cast<uint8_t>(OpCode::LOOP_END),
            static_cast<uint8_t>(OpCode::PROFILE_ENTER),
            static_cast<uint8_t>(OpCode::PROFILE_EXIT),
            static_cast<uint8_t>(OpCode::HALT),
        };
        return supported;
    }

    bool JitCompiler::canCompile(const bytecode::BytecodeProgram::FunctionMetadata& meta,
                                  const bytecode::BytecodeProgram& program) const
    {
        if (meta.isNative || meta.isAsync)
            return false;

        const auto& supported = getSupportedOpcodes();
        size_t endOffset = meta.startOffset + meta.instructionCount;
        for (size_t ip = meta.startOffset; ip < endOffset; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            if (supported.find(static_cast<uint8_t>(instr.opcode)) == supported.end())
                return false;
        }
        return true;
    }

    bool JitCompiler::compile(const std::string& functionName,
                               const bytecode::BytecodeProgram& program,
                               JitCodeCache& codeCache)
    {
        if (codeCache.contains(functionName))
            return true;

        const auto* funcMeta = program.getFunction(functionName);
        if (!funcMeta || !canCompile(*funcMeta, program))
        {
            bailoutCount++;
            return false;
        }

        CodeHolder code;
        code.init(codeCache.getRuntime().environment());
        Compiler cc(&code);

        FuncNode* func = cc.add_func(FuncSignature::build<void, JitContext*>());
        Gp ctxPtr = cc.new_gp64("ctx");
        func->set_arg(0, ctxPtr);

        size_t localCount = funcMeta->localCount;
        if (localCount == 0) localCount = 1;
        Mem localsArea = cc.new_stack(static_cast<uint32_t>(localCount * 8), 8);
        Gp localsBase = cc.new_gp64("localsBase");
        cc.lea(localsBase, localsArea);

        constexpr size_t MAX_OP_STACK = 64;
        Mem stackArea = cc.new_stack(MAX_OP_STACK * 8, 8);
        Gp stackBase = cc.new_gp64("stackBase");
        cc.lea(stackBase, stackArea);

        // Compile-time type tracking
        std::vector<SlotType> slotTypes;
        std::unordered_map<size_t, SlotType> localTypes;
        bool compileFailed = false;
        constexpr size_t valueSize = sizeof(value::Value);

        // Unbox arguments with type awareness
        {
            Gp argsPtr = cc.new_gp64("argsPtr");
            cc.mov(argsPtr, Mem(ctxPtr, offsetof(JitContext, args)));

            for (size_t i = 0; i < funcMeta->parameterCount; ++i)
            {
                SlotType paramType = SlotType::INT;
                if (i < funcMeta->parameterTypes.size())
                {
                    const std::string& t = funcMeta->parameterTypes[i];
                    if (t == "float") paramType = SlotType::FLOAT;
                    else if (t == "bool") paramType = SlotType::BOOL;
                }
                localTypes[i] = paramType;

                Gp argAddr = cc.new_gp64();
                cc.lea(argAddr, Mem(argsPtr, static_cast<int32_t>(i * valueSize)));

                if (paramType == SlotType::FLOAT)
                {
                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_float),
                              FuncSignature::build<float, const value::Value*>());
                    inv->set_arg(0, argAddr);
                    Vec val = cc.new_xmm();
                    inv->set_ret(0, val);
                    cc.movss(Mem(localsBase, static_cast<int32_t>(i * 8)), val);
                }
                else
                {
                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_int),
                              FuncSignature::build<int64_t, const value::Value*>());
                    inv->set_arg(0, argAddr);
                    Gp val = cc.new_gp64();
                    inv->set_ret(0, val);
                    cc.mov(Mem(localsBase, static_cast<int32_t>(i * 8)), val);
                }
            }
        }

        for (size_t i = funcMeta->parameterCount; i < funcMeta->localCount; ++i)
            cc.mov(Mem(localsBase, static_cast<int32_t>(i * 8)), 0);

        // Create jump target labels
        size_t startOffset = funcMeta->startOffset;
        size_t instrCount = funcMeta->instructionCount;
        std::unordered_map<size_t, Label> labels;

        for (size_t ip = startOffset; ip < startOffset + instrCount; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            if (instr.opcode == OpCode::JUMP || instr.opcode == OpCode::JUMP_IF_FALSE ||
                instr.opcode == OpCode::JUMP_IF_TRUE || instr.opcode == OpCode::JUMP_BACK)
            {
                if (!instr.operands.empty())
                {
                    size_t target = instr.operands[0];
                    if (labels.find(target) == labels.end())
                        labels[target] = cc.new_label();
                }
            }
        }

        // Helper: box operand stack value into Value*
        auto emitBox = [&](Gp destAddr, int stackOff, SlotType type) {
            if (type == SlotType::FLOAT)
            {
                Vec val = cc.new_xmm();
                cc.movss(val, Mem(stackBase, stackOff * 8));
                InvokeNode* inv;
                cc.invoke(Out(inv), (uint64_t)jit_box_float,
                          FuncSignature::build<void, value::Value*, float>());
                inv->set_arg(0, destAddr);
                inv->set_arg(1, val);
            }
            else
            {
                Gp val = cc.new_gp64();
                cc.mov(val, Mem(stackBase, stackOff * 8));
                uint64_t fn = (type == SlotType::BOOL) ? (uint64_t)jit_box_bool : (uint64_t)jit_box_int;
                InvokeNode* inv;
                cc.invoke(Out(inv), fn, FuncSignature::build<void, value::Value*, int64_t>());
                inv->set_arg(0, destAddr);
                inv->set_arg(1, val);
            }
        };

        // Helper: unbox Value* into operand stack slot
        auto emitUnbox = [&](Gp srcAddr, int stackOff, SlotType type) {
            if (type == SlotType::FLOAT)
            {
                InvokeNode* inv;
                cc.invoke(Out(inv), (uint64_t)jit_unbox_float,
                          FuncSignature::build<float, const value::Value*>());
                inv->set_arg(0, srcAddr);
                Vec val = cc.new_xmm();
                inv->set_ret(0, val);
                cc.movss(Mem(stackBase, stackOff * 8), val);
            }
            else
            {
                InvokeNode* inv;
                cc.invoke(Out(inv), (uint64_t)jit_unbox_int,
                          FuncSignature::build<int64_t, const value::Value*>());
                inv->set_arg(0, srcAddr);
                Gp val = cc.new_gp64();
                inv->set_ret(0, val);
                cc.mov(Mem(stackBase, stackOff * 8), val);
            }
        };

        // Helper: safe type pop from slotTypes
        auto popType = [&]() -> SlotType {
            if (slotTypes.empty()) return SlotType::INT;
            SlotType t = slotTypes.back();
            slotTypes.pop_back();
            return t;
        };

        auto topType = [&]() -> SlotType {
            return slotTypes.empty() ? SlotType::INT : slotTypes.back();
        };

        // Compile-time operand stack depth (must be declared before lambdas that use it)
        int stackDepth = 0;

        // Helper: generic binary op via C++ helper
        auto emitGenericBinop = [&](uint64_t helperFn, SlotType lType, SlotType rType) {
            Gp leftAddr = cc.new_gp64();
            cc.lea(leftAddr, Mem(ctxPtr, offsetof(JitContext, callArgs)));
            emitBox(leftAddr, stackDepth - 1, lType);

            Gp rightAddr = cc.new_gp64();
            cc.lea(rightAddr, Mem(ctxPtr, offsetof(JitContext, callArgs)
                                  + static_cast<int32_t>(valueSize)));
            emitBox(rightAddr, stackDepth, rType);

            Gp resultAddr = cc.new_gp64();
            cc.lea(resultAddr, Mem(ctxPtr, offsetof(JitContext, callArgs)
                                    + static_cast<int32_t>(2 * valueSize)));

            InvokeNode* inv;
            cc.invoke(Out(inv), helperFn,
                      FuncSignature::build<void, value::Value*, const value::Value*, const value::Value*>());
            inv->set_arg(0, resultAddr);
            inv->set_arg(1, leftAddr);
            inv->set_arg(2, rightAddr);

            SlotType resType = (lType == SlotType::FLOAT || rType == SlotType::FLOAT)
                             ? SlotType::FLOAT : SlotType::INT;
            emitUnbox(resultAddr, stackDepth - 1, resType);
            slotTypes.push_back(resType);
        };

        // Helper: emit comparison
        enum class CmpOp { EQ, NE, LT, GT, LE, GE };
        auto emitCmp = [&](CmpOp kind) {
            stackDepth--;
            SlotType rType = popType();
            SlotType lType = popType();

            Gp result = cc.new_gp64();
            cc.xor_(result, result);

            if (lType == SlotType::FLOAT || rType == SlotType::FLOAT)
            {
                Vec right = cc.new_xmm();
                Vec left = cc.new_xmm();
                cc.movss(right, Mem(stackBase, stackDepth * 8));
                cc.movss(left, Mem(stackBase, (stackDepth - 1) * 8));
                cc.ucomiss(left, right);
                switch (kind) {
                    case CmpOp::EQ: cc.sete(result.r8()); break;
                    case CmpOp::NE: cc.setne(result.r8()); break;
                    case CmpOp::LT: cc.setb(result.r8()); break;
                    case CmpOp::GT: cc.seta(result.r8()); break;
                    case CmpOp::LE: cc.setbe(result.r8()); break;
                    case CmpOp::GE: cc.setae(result.r8()); break;
                }
            }
            else
            {
                Gp right = cc.new_gp64();
                Gp left = cc.new_gp64();
                cc.mov(right, Mem(stackBase, stackDepth * 8));
                cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                cc.cmp(left, right);
                switch (kind) {
                    case CmpOp::EQ: cc.sete(result.r8()); break;
                    case CmpOp::NE: cc.setne(result.r8()); break;
                    case CmpOp::LT: cc.setl(result.r8()); break;
                    case CmpOp::GT: cc.setg(result.r8()); break;
                    case CmpOp::LE: cc.setle(result.r8()); break;
                    case CmpOp::GE: cc.setge(result.r8()); break;
                }
            }
            cc.mov(Mem(stackBase, (stackDepth - 1) * 8), result);
            slotTypes.push_back(SlotType::BOOL);
        };

        // Main codegen loop
        for (size_t ip = startOffset; ip < startOffset + instrCount && !compileFailed; ++ip)
        {
            auto labelIt = labels.find(ip);
            if (labelIt != labels.end())
                cc.bind(labelIt->second);

            const auto& instr = program.getInstruction(ip);

            switch (instr.opcode)
            {
                case OpCode::LINE:
                case OpCode::SOURCE_FILE:
                case OpCode::NOP:
                case OpCode::LOOP_START:
                case OpCode::LOOP_END:
                case OpCode::PROFILE_ENTER:
                case OpCode::PROFILE_EXIT:
                case OpCode::HALT:
                    break;

                // ===== Stack Operations =====
                case OpCode::PUSH_INT:
                {
                    int64_t val = program.getConstantPool().getInteger(instr.operands[0]);
                    Gp tmp = cc.new_gp64();
                    cc.mov(tmp, val);
                    cc.mov(Mem(stackBase, stackDepth * 8), tmp);
                    slotTypes.push_back(SlotType::INT);
                    stackDepth++;
                    break;
                }

                case OpCode::PUSH_FLOAT:
                {
                    double dval = program.getConstantPool().getFloat(instr.operands[0]);
                    float fval = static_cast<float>(dval);
                    uint32_t bits;
                    std::memcpy(&bits, &fval, sizeof(bits));
                    Gp tmp = cc.new_gp64();
                    cc.mov(tmp, static_cast<int64_t>(bits));
                    cc.mov(Mem(stackBase, stackDepth * 8), tmp);
                    slotTypes.push_back(SlotType::FLOAT);
                    stackDepth++;
                    break;
                }

                case OpCode::PUSH_BOOL:
                {
                    int64_t val = (instr.operands[0] != 0) ? 1 : 0;
                    Gp tmp = cc.new_gp64();
                    cc.mov(tmp, val);
                    cc.mov(Mem(stackBase, stackDepth * 8), tmp);
                    slotTypes.push_back(SlotType::BOOL);
                    stackDepth++;
                    break;
                }

                case OpCode::PUSH_NULL:
                {
                    cc.mov(Mem(stackBase, stackDepth * 8), 0);
                    slotTypes.push_back(SlotType::INT);
                    stackDepth++;
                    break;
                }

                case OpCode::POP:
                    stackDepth--;
                    popType();
                    break;

                case OpCode::DUP:
                {
                    Gp tmp = cc.new_gp64();
                    cc.mov(tmp, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.mov(Mem(stackBase, stackDepth * 8), tmp);
                    slotTypes.push_back(topType());
                    stackDepth++;
                    break;
                }

                case OpCode::SWAP:
                {
                    Gp a = cc.new_gp64();
                    Gp b = cc.new_gp64();
                    cc.mov(a, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.mov(b, Mem(stackBase, (stackDepth - 2) * 8));
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), b);
                    cc.mov(Mem(stackBase, (stackDepth - 2) * 8), a);
                    if (slotTypes.size() >= 2)
                        std::swap(slotTypes[slotTypes.size() - 1], slotTypes[slotTypes.size() - 2]);
                    break;
                }

                // ===== Integer Arithmetic =====
                case OpCode::ADD_INT:
                {
                    stackDepth--;
                    popType(); popType();
                    Gp right = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));
                    cc.add(Mem(stackBase, (stackDepth - 1) * 8), right);
                    slotTypes.push_back(SlotType::INT);
                    break;
                }

                case OpCode::SUB_INT:
                {
                    stackDepth--;
                    popType(); popType();
                    Gp right = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));
                    cc.sub(Mem(stackBase, (stackDepth - 1) * 8), right);
                    slotTypes.push_back(SlotType::INT);
                    break;
                }

                case OpCode::MUL_INT:
                {
                    stackDepth--;
                    popType(); popType();
                    Gp left = cc.new_gp64();
                    Gp right = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));
                    cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.imul(left, right);
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), left);
                    slotTypes.push_back(SlotType::INT);
                    break;
                }

                case OpCode::DIV_INT:
                {
                    stackDepth--;
                    popType(); popType();
                    Gp right = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));
                    cc.test(right, right);
                    Label notZero = cc.new_label();
                    cc.jnz(notZero);
                    InvokeNode* invDZ;
                    cc.invoke(Out(invDZ), reinterpret_cast<uint64_t>(jit_throw_div_by_zero),
                              FuncSignature::build<void>());
                    cc.bind(notZero);
                    Gp left = cc.new_gp64();
                    cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                    Gp raxReg = cc.new_gp64();
                    Gp rdxReg = cc.new_gp64();
                    cc.mov(raxReg, left);
                    cc.cqo(rdxReg, raxReg);
                    cc.idiv(rdxReg, raxReg, right);
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), raxReg);
                    slotTypes.push_back(SlotType::INT);
                    break;
                }

                case OpCode::NEG:
                    cc.neg(Mem(stackBase, (stackDepth - 1) * 8));
                    break;

                case OpCode::INC:
                {
                    Gp tmp = cc.new_gp64();
                    cc.mov(tmp, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.inc(tmp);
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), tmp);
                    break;
                }

                case OpCode::DEC:
                {
                    Gp tmp = cc.new_gp64();
                    cc.mov(tmp, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.dec(tmp);
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), tmp);
                    break;
                }

                // ===== Float Arithmetic =====
                case OpCode::ADD_FLOAT:
                {
                    stackDepth--;
                    popType(); popType();
                    Vec right = cc.new_xmm();
                    Vec left = cc.new_xmm();
                    cc.movss(right, Mem(stackBase, stackDepth * 8));
                    cc.movss(left, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.addss(left, right);
                    cc.movss(Mem(stackBase, (stackDepth - 1) * 8), left);
                    slotTypes.push_back(SlotType::FLOAT);
                    break;
                }

                case OpCode::SUB_FLOAT:
                {
                    stackDepth--;
                    popType(); popType();
                    Vec right = cc.new_xmm();
                    Vec left = cc.new_xmm();
                    cc.movss(right, Mem(stackBase, stackDepth * 8));
                    cc.movss(left, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.subss(left, right);
                    cc.movss(Mem(stackBase, (stackDepth - 1) * 8), left);
                    slotTypes.push_back(SlotType::FLOAT);
                    break;
                }

                case OpCode::MUL_FLOAT:
                {
                    stackDepth--;
                    popType(); popType();
                    Vec right = cc.new_xmm();
                    Vec left = cc.new_xmm();
                    cc.movss(right, Mem(stackBase, stackDepth * 8));
                    cc.movss(left, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.mulss(left, right);
                    cc.movss(Mem(stackBase, (stackDepth - 1) * 8), left);
                    slotTypes.push_back(SlotType::FLOAT);
                    break;
                }

                case OpCode::DIV_FLOAT:
                {
                    stackDepth--;
                    popType(); popType();
                    Vec right = cc.new_xmm();
                    Vec left = cc.new_xmm();
                    cc.movss(right, Mem(stackBase, stackDepth * 8));
                    cc.movss(left, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.divss(left, right);
                    cc.movss(Mem(stackBase, (stackDepth - 1) * 8), left);
                    slotTypes.push_back(SlotType::FLOAT);
                    break;
                }

                // ===== Generic Arithmetic =====
                case OpCode::ADD:
                {
                    SlotType rType = popType();
                    SlotType lType = popType();
                    stackDepth--;

                    if (lType == SlotType::INT && rType == SlotType::INT)
                    {
                        Gp right = cc.new_gp64();
                        cc.mov(right, Mem(stackBase, stackDepth * 8));
                        cc.add(Mem(stackBase, (stackDepth - 1) * 8), right);
                        slotTypes.push_back(SlotType::INT);
                    }
                    else if (lType == SlotType::FLOAT && rType == SlotType::FLOAT)
                    {
                        Vec right = cc.new_xmm();
                        Vec left = cc.new_xmm();
                        cc.movss(right, Mem(stackBase, stackDepth * 8));
                        cc.movss(left, Mem(stackBase, (stackDepth - 1) * 8));
                        cc.addss(left, right);
                        cc.movss(Mem(stackBase, (stackDepth - 1) * 8), left);
                        slotTypes.push_back(SlotType::FLOAT);
                    }
                    else
                    {
                        emitGenericBinop((uint64_t)jit_generic_add, lType, rType);
                    }
                    break;
                }

                case OpCode::SUB:
                {
                    SlotType rType = popType();
                    SlotType lType = popType();
                    stackDepth--;

                    if (lType == SlotType::INT && rType == SlotType::INT)
                    {
                        Gp right = cc.new_gp64();
                        cc.mov(right, Mem(stackBase, stackDepth * 8));
                        cc.sub(Mem(stackBase, (stackDepth - 1) * 8), right);
                        slotTypes.push_back(SlotType::INT);
                    }
                    else if (lType == SlotType::FLOAT && rType == SlotType::FLOAT)
                    {
                        Vec right = cc.new_xmm();
                        Vec left = cc.new_xmm();
                        cc.movss(right, Mem(stackBase, stackDepth * 8));
                        cc.movss(left, Mem(stackBase, (stackDepth - 1) * 8));
                        cc.subss(left, right);
                        cc.movss(Mem(stackBase, (stackDepth - 1) * 8), left);
                        slotTypes.push_back(SlotType::FLOAT);
                    }
                    else
                    {
                        emitGenericBinop((uint64_t)jit_generic_sub, lType, rType);
                    }
                    break;
                }

                case OpCode::MUL:
                {
                    SlotType rType = popType();
                    SlotType lType = popType();
                    stackDepth--;

                    if (lType == SlotType::INT && rType == SlotType::INT)
                    {
                        Gp left = cc.new_gp64();
                        Gp right = cc.new_gp64();
                        cc.mov(right, Mem(stackBase, stackDepth * 8));
                        cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                        cc.imul(left, right);
                        cc.mov(Mem(stackBase, (stackDepth - 1) * 8), left);
                        slotTypes.push_back(SlotType::INT);
                    }
                    else if (lType == SlotType::FLOAT && rType == SlotType::FLOAT)
                    {
                        Vec right = cc.new_xmm();
                        Vec left = cc.new_xmm();
                        cc.movss(right, Mem(stackBase, stackDepth * 8));
                        cc.movss(left, Mem(stackBase, (stackDepth - 1) * 8));
                        cc.mulss(left, right);
                        cc.movss(Mem(stackBase, (stackDepth - 1) * 8), left);
                        slotTypes.push_back(SlotType::FLOAT);
                    }
                    else
                    {
                        emitGenericBinop((uint64_t)jit_generic_mul, lType, rType);
                    }
                    break;
                }

                case OpCode::DIV:
                {
                    SlotType rType = popType();
                    SlotType lType = popType();
                    stackDepth--;

                    if (lType == SlotType::INT && rType == SlotType::INT)
                    {
                        Gp right = cc.new_gp64();
                        cc.mov(right, Mem(stackBase, stackDepth * 8));
                        cc.test(right, right);
                        Label nz = cc.new_label();
                        cc.jnz(nz);
                        InvokeNode* dz;
                        cc.invoke(Out(dz), (uint64_t)jit_throw_div_by_zero,
                                  FuncSignature::build<void>());
                        cc.bind(nz);
                        Gp left = cc.new_gp64();
                        cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                        Gp rax = cc.new_gp64();
                        Gp rdx = cc.new_gp64();
                        cc.mov(rax, left);
                        cc.cqo(rdx, rax);
                        cc.idiv(rdx, rax, right);
                        cc.mov(Mem(stackBase, (stackDepth - 1) * 8), rax);
                        slotTypes.push_back(SlotType::INT);
                    }
                    else
                    {
                        emitGenericBinop((uint64_t)jit_generic_div, lType, rType);
                    }
                    break;
                }

                case OpCode::MOD:
                {
                    SlotType rType = popType();
                    SlotType lType = popType();
                    stackDepth--;

                    if (lType == SlotType::INT && rType == SlotType::INT)
                    {
                        Gp right = cc.new_gp64();
                        cc.mov(right, Mem(stackBase, stackDepth * 8));
                        cc.test(right, right);
                        Label nz = cc.new_label();
                        cc.jnz(nz);
                        InvokeNode* dz;
                        cc.invoke(Out(dz), (uint64_t)jit_throw_div_by_zero,
                                  FuncSignature::build<void>());
                        cc.bind(nz);
                        Gp left = cc.new_gp64();
                        cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                        Gp rax = cc.new_gp64();
                        Gp rdx = cc.new_gp64();
                        cc.mov(rax, left);
                        cc.cqo(rdx, rax);
                        cc.idiv(rdx, rax, right);
                        cc.mov(Mem(stackBase, (stackDepth - 1) * 8), rdx);
                        slotTypes.push_back(SlotType::INT);
                    }
                    else
                    {
                        emitGenericBinop((uint64_t)jit_generic_mod, lType, rType);
                    }
                    break;
                }

                // ===== Comparisons =====
                case OpCode::EQ:  case OpCode::EQ_INT: emitCmp(CmpOp::EQ); break;
                case OpCode::NE:  case OpCode::NE_INT: emitCmp(CmpOp::NE); break;
                case OpCode::LT:  case OpCode::LT_INT: emitCmp(CmpOp::LT); break;
                case OpCode::GT:  case OpCode::GT_INT: emitCmp(CmpOp::GT); break;
                case OpCode::LE:                        emitCmp(CmpOp::LE); break;
                case OpCode::GE:                        emitCmp(CmpOp::GE); break;

                // ===== Logical =====
                case OpCode::NOT:
                {
                    Gp val = cc.new_gp64();
                    Gp result = cc.new_gp64();
                    cc.mov(val, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.xor_(result, result);
                    cc.test(val, val);
                    cc.sete(result.r8());
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), result);
                    if (!slotTypes.empty()) slotTypes.back() = SlotType::BOOL;
                    break;
                }

                case OpCode::AND:
                {
                    stackDepth--;
                    popType(); popType();
                    Gp right = cc.new_gp64();
                    Gp left = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));
                    cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                    Gp r1 = cc.new_gp64();
                    Gp r2 = cc.new_gp64();
                    cc.xor_(r1, r1);
                    cc.test(left, left);
                    cc.setne(r1.r8());
                    cc.xor_(r2, r2);
                    cc.test(right, right);
                    cc.setne(r2.r8());
                    cc.and_(r1, r2);
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), r1);
                    slotTypes.push_back(SlotType::BOOL);
                    break;
                }

                case OpCode::OR:
                {
                    stackDepth--;
                    popType(); popType();
                    Gp right = cc.new_gp64();
                    Gp left = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));
                    cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                    Gp r1 = cc.new_gp64();
                    Gp r2 = cc.new_gp64();
                    cc.xor_(r1, r1);
                    cc.test(left, left);
                    cc.setne(r1.r8());
                    cc.xor_(r2, r2);
                    cc.test(right, right);
                    cc.setne(r2.r8());
                    cc.or_(r1, r2);
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), r1);
                    slotTypes.push_back(SlotType::BOOL);
                    break;
                }

                // ===== Variables =====
                case OpCode::LOAD_LOCAL:
                {
                    size_t slot = instr.operands[0];
                    Gp tmp = cc.new_gp64();
                    cc.mov(tmp, Mem(localsBase, static_cast<int32_t>(slot * 8)));
                    cc.mov(Mem(stackBase, stackDepth * 8), tmp);
                    SlotType lt = localTypes.count(slot) ? localTypes[slot] : SlotType::INT;
                    slotTypes.push_back(lt);
                    stackDepth++;
                    break;
                }

                case OpCode::STORE_LOCAL:
                {
                    size_t slot = instr.operands[0];
                    Gp tmp = cc.new_gp64();
                    cc.mov(tmp, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.mov(Mem(localsBase, static_cast<int32_t>(slot * 8)), tmp);
                    localTypes[slot] = topType();
                    break;
                }

                // ===== Control Flow =====
                case OpCode::JUMP:
                    cc.jmp(labels[instr.operands[0]]);
                    break;

                case OpCode::JUMP_IF_FALSE:
                {
                    stackDepth--;
                    popType();
                    Gp cond = cc.new_gp64();
                    cc.mov(cond, Mem(stackBase, stackDepth * 8));
                    cc.test(cond, cond);
                    cc.jz(labels[instr.operands[0]]);
                    break;
                }

                case OpCode::JUMP_IF_TRUE:
                {
                    stackDepth--;
                    popType();
                    Gp cond = cc.new_gp64();
                    cc.mov(cond, Mem(stackBase, stackDepth * 8));
                    cc.test(cond, cond);
                    cc.jnz(labels[instr.operands[0]]);
                    break;
                }

                case OpCode::JUMP_BACK:
                {
                    InvokeNode* gc;
                    cc.invoke(Out(gc), reinterpret_cast<uint64_t>(jit_gc_safepoint),
                              FuncSignature::build<void>());
                    cc.jmp(labels[instr.operands[0]]);
                    break;
                }

                case OpCode::RETURN:
                    cc.mov(Mem(ctxPtr, offsetof(JitContext, hasReturnValue)), 0);
                    cc.ret();
                    break;

                case OpCode::RETURN_VALUE:
                {
                    stackDepth--;
                    SlotType retType = popType();

                    if (retType == SlotType::FLOAT)
                    {
                        Vec retVal = cc.new_xmm();
                        cc.movss(retVal, Mem(stackBase, stackDepth * 8));
                        InvokeNode* inv;
                        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_float),
                                  FuncSignature::build<void, JitContext*, float>());
                        inv->set_arg(0, ctxPtr);
                        inv->set_arg(1, retVal);
                    }
                    else if (retType == SlotType::BOOL)
                    {
                        Gp retVal = cc.new_gp64();
                        cc.mov(retVal, Mem(stackBase, stackDepth * 8));
                        InvokeNode* inv;
                        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_bool),
                                  FuncSignature::build<void, JitContext*, int64_t>());
                        inv->set_arg(0, ctxPtr);
                        inv->set_arg(1, retVal);
                    }
                    else
                    {
                        Gp retVal = cc.new_gp64();
                        cc.mov(retVal, Mem(stackBase, stackDepth * 8));
                        InvokeNode* inv;
                        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_int),
                                  FuncSignature::build<void, JitContext*, int64_t>());
                        inv->set_arg(0, ctxPtr);
                        inv->set_arg(1, retVal);
                    }
                    cc.ret();
                    break;
                }

                // ===== Function Calls =====
                case OpCode::CALL:
                {
                    uint32_t nameIndex = static_cast<uint32_t>(instr.operands[0]);
                    size_t argCount = instr.operands[1];

                    if (argCount > JitContext::MAX_CALL_ARGS)
                    {
                        compileFailed = true;
                        break;
                    }

                    const std::string& funcName = program.getConstantPool().getString(nameIndex);

                    const auto* calleeMeta = program.getFunction(funcName);
                    std::string returnType;
                    if (calleeMeta)
                    {
                        returnType = calleeMeta->returnType;
                    }
                    else
                    {
                        returnType = getKnownNativeReturnType(funcName);
                        if (returnType.empty())
                        {
                            compileFailed = true;
                            break;
                        }
                    }

                    if (returnType != "int" && returnType != "float" &&
                        returnType != "bool" && returnType != "void")
                    {
                        compileFailed = true;
                        break;
                    }

                    // Box arguments into ctx->callArgs[]
                    for (size_t i = 0; i < argCount; ++i)
                    {
                        int stackIdx = stackDepth - static_cast<int>(argCount) + static_cast<int>(i);
                        SlotType argType = (stackIdx >= 0 && stackIdx < static_cast<int>(slotTypes.size()))
                                         ? slotTypes[stackIdx] : SlotType::INT;
                        Gp destAddr = cc.new_gp64();
                        cc.lea(destAddr, Mem(ctxPtr, offsetof(JitContext, callArgs)
                                              + static_cast<int32_t>(i * valueSize)));
                        emitBox(destAddr, stackIdx, argType);
                    }

                    // Pop arguments
                    for (size_t i = 0; i < argCount; ++i)
                        popType();
                    stackDepth -= static_cast<int>(argCount);

                    // Call jit_call_function(ctx, nameIndex, argCount)
                    // Arguments must be defined BEFORE the invoke node
                    Gp niReg = cc.new_gp64();
                    cc.mov(niReg, static_cast<int64_t>(nameIndex));
                    Gp acReg = cc.new_gp64();
                    cc.mov(acReg, static_cast<int64_t>(argCount));

                    InvokeNode* callInv;
                    cc.invoke(Out(callInv), reinterpret_cast<uint64_t>(jit_call_function),
                              FuncSignature::build<void, JitContext*, uint32_t, size_t>());
                    callInv->set_arg(0, ctxPtr);
                    callInv->set_arg(1, niReg);
                    callInv->set_arg(2, acReg);

                    // Unbox return value
                    if (returnType != "void")
                    {
                        SlotType retSlot = SlotType::INT;
                        if (returnType == "float") retSlot = SlotType::FLOAT;
                        else if (returnType == "bool") retSlot = SlotType::BOOL;

                        Gp retAddr = cc.new_gp64();
                        cc.lea(retAddr, Mem(ctxPtr, offsetof(JitContext, returnValue)));
                        emitUnbox(retAddr, stackDepth, retSlot);
                        slotTypes.push_back(retSlot);
                        stackDepth++;
                    }
                    break;
                }

                default:
                    break;
            }
        }

        if (compileFailed)
        {
            bailoutCount++;
            return false;
        }

        cc.mov(Mem(ctxPtr, offsetof(JitContext, hasReturnValue)), 0);
        cc.end_func();

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

        codeCache.store(functionName, fn);
        compileCount++;
        return true;
    }
}
