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

            static_cast<uint8_t>(OpCode::PUSH_STRING),

            static_cast<uint8_t>(OpCode::GET_FIELD),
            static_cast<uint8_t>(OpCode::SET_FIELD),

            static_cast<uint8_t>(OpCode::CALL_METHOD),
            static_cast<uint8_t>(OpCode::CALL_STATIC),

            static_cast<uint8_t>(OpCode::NEW_OBJECT),
            static_cast<uint8_t>(OpCode::NEW_ARRAY),
            static_cast<uint8_t>(OpCode::ARRAY_GET),
            static_cast<uint8_t>(OpCode::ARRAY_SET),
            static_cast<uint8_t>(OpCode::ARRAY_LENGTH),

            static_cast<uint8_t>(OpCode::INSTANCEOF),
            static_cast<uint8_t>(OpCode::CAST),

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
        constexpr size_t MAX_OP_STACK = 64;
        constexpr size_t valueSize = sizeof(value::Value);

        // Pre-scan: determine if function uses boxed types (Phase 4 opcodes)
        bool usesBoxedTypes = false;
        {
            size_t scanEnd = funcMeta->startOffset + funcMeta->instructionCount;
            for (size_t ip = funcMeta->startOffset; ip < scanEnd; ++ip)
            {
                const auto& si = program.getInstruction(ip);
                switch (si.opcode)
                {
                    case OpCode::PUSH_STRING:
                    case OpCode::GET_FIELD:
                    case OpCode::SET_FIELD:
                    case OpCode::NEW_OBJECT:
                    case OpCode::CALL_METHOD:
                    case OpCode::CALL_STATIC:
                    case OpCode::NEW_ARRAY:
                    case OpCode::ARRAY_GET:
                    case OpCode::ARRAY_SET:
                    case OpCode::ARRAY_LENGTH:
                    case OpCode::INSTANCEOF:
                    case OpCode::CAST:
                        usesBoxedTypes = true;
                        break;
                    default:
                        break;
                }
                if (usesBoxedTypes) break;
            }
            // Also check if CALL returns non-primitive
            if (!usesBoxedTypes)
            {
                for (size_t ip = funcMeta->startOffset; ip < scanEnd; ++ip)
                {
                    const auto& si = program.getInstruction(ip);
                    if (si.opcode == OpCode::CALL && si.operands.size() >= 2)
                    {
                        const std::string& fn = program.getConstantPool().getString(
                            static_cast<uint32_t>(si.operands[0]));
                        const auto* cm = program.getFunction(fn);
                        if (cm && cm->returnType != "int" && cm->returnType != "float" &&
                            cm->returnType != "bool" && cm->returnType != "void")
                        {
                            usesBoxedTypes = true;
                            break;
                        }
                    }
                }
            }
            // Also check parameter types
            if (!usesBoxedTypes)
            {
                for (const auto& t : funcMeta->parameterTypes)
                {
                    if (t != "int" && t != "float" && t != "bool")
                    {
                        usesBoxedTypes = true;
                        break;
                    }
                }
            }
        }

        const size_t localStride = usesBoxedTypes ? valueSize : 8;

        Mem localsArea = cc.new_stack(static_cast<uint32_t>(localCount * localStride), 8);
        Gp localsBase = cc.new_gp64("localsBase");
        cc.lea(localsBase, localsArea);

        Mem stackArea = cc.new_stack(MAX_OP_STACK * 8, 8);
        Gp stackBase = cc.new_gp64("stackBase");
        cc.lea(stackBase, stackArea);

        // Boxed value stack (only allocated when function uses boxed types)
        Gp boxedBase = cc.new_gp64("boxedBase");
        if (usesBoxedTypes)
        {
            Mem boxedArea = cc.new_stack(static_cast<uint32_t>(MAX_OP_STACK * valueSize), 8);
            cc.lea(boxedBase, boxedArea);
        }

        // Program pointer for constant pool access in Phase 4 helpers
        Gp progPtr = cc.new_gp64("progPtr");
        if (usesBoxedTypes)
        {
            cc.mov(progPtr, reinterpret_cast<uint64_t>(&program));
        }

        // Compile-time type tracking
        std::vector<SlotType> slotTypes;
        std::unordered_map<size_t, SlotType> localTypes;
        bool compileFailed = false;

        // Initialize locals and boxed stacks
        if (usesBoxedTypes)
        {
            // Init all locals as monostate Values
            Gp initBase = cc.new_gp64();
            cc.mov(initBase, localsBase);
            Gp initCount = cc.new_gp64();
            cc.mov(initCount, static_cast<int64_t>(localCount));
            InvokeNode* initLocals;
            cc.invoke(Out(initLocals), reinterpret_cast<uint64_t>(jit_locals_init),
                      FuncSignature::build<void, value::Value*, size_t>());
            initLocals->set_arg(0, initBase);
            initLocals->set_arg(1, initCount);

            // Init all boxed stack slots as monostate Values
            Gp bsBase = cc.new_gp64();
            cc.mov(bsBase, boxedBase);
            Gp bsCount = cc.new_gp64();
            cc.mov(bsCount, static_cast<int64_t>(MAX_OP_STACK));
            InvokeNode* initBoxed;
            cc.invoke(Out(initBoxed), reinterpret_cast<uint64_t>(jit_locals_init),
                      FuncSignature::build<void, value::Value*, size_t>());
            initBoxed->set_arg(0, bsBase);
            initBoxed->set_arg(1, bsCount);
        }

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
                    else if (t != "int") paramType = SlotType::BOXED;
                }
                localTypes[i] = paramType;

                Gp argAddr = cc.new_gp64();
                cc.lea(argAddr, Mem(argsPtr, static_cast<int32_t>(i * valueSize)));

                if (isBoxedSlotType(paramType))
                {
                    // Boxed parameter: copy full Value into local
                    Gp destAddr = cc.new_gp64();
                    cc.lea(destAddr, Mem(localsBase, static_cast<int32_t>(i * localStride)));
                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                              FuncSignature::build<void, value::Value*, const value::Value*>());
                    inv->set_arg(0, destAddr);
                    inv->set_arg(1, argAddr);
                }
                else if (usesBoxedTypes)
                {
                    // Primitive param in boxed mode: unbox, then box into Value-sized local
                    if (paramType == SlotType::FLOAT)
                    {
                        InvokeNode* inv;
                        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_float),
                                  FuncSignature::build<float, const value::Value*>());
                        inv->set_arg(0, argAddr);
                        Vec val = cc.new_xmm();
                        inv->set_ret(0, val);
                        // Box float into Value-sized local
                        Gp destAddr = cc.new_gp64();
                        cc.lea(destAddr, Mem(localsBase, static_cast<int32_t>(i * localStride)));
                        InvokeNode* boxInv;
                        cc.invoke(Out(boxInv), reinterpret_cast<uint64_t>(jit_box_float),
                                  FuncSignature::build<void, value::Value*, float>());
                        boxInv->set_arg(0, destAddr);
                        boxInv->set_arg(1, val);
                    }
                    else
                    {
                        InvokeNode* inv;
                        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_int),
                                  FuncSignature::build<int64_t, const value::Value*>());
                        inv->set_arg(0, argAddr);
                        Gp val = cc.new_gp64();
                        inv->set_ret(0, val);
                        // Box int/bool into Value-sized local
                        Gp destAddr = cc.new_gp64();
                        cc.lea(destAddr, Mem(localsBase, static_cast<int32_t>(i * localStride)));
                        uint64_t boxFn = (paramType == SlotType::BOOL)
                            ? reinterpret_cast<uint64_t>(jit_box_bool)
                            : reinterpret_cast<uint64_t>(jit_box_int);
                        InvokeNode* boxInv;
                        cc.invoke(Out(boxInv), boxFn,
                                  FuncSignature::build<void, value::Value*, int64_t>());
                        boxInv->set_arg(0, destAddr);
                        boxInv->set_arg(1, val);
                    }
                }
                else
                {
                    // Phase 3 path: primitive param, 8-byte locals
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
        }

        // Zero-init non-parameter locals (only in non-boxed mode; boxed mode already inited)
        if (!usesBoxedTypes)
        {
            for (size_t i = funcMeta->parameterCount; i < funcMeta->localCount; ++i)
                cc.mov(Mem(localsBase, static_cast<int32_t>(i * 8)), 0);
        }

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
        // Helper: emit cleanup for boxed stacks and locals at function exit
        auto emitCleanup = [&]() {
            if (!usesBoxedTypes) return;
            // Clean up all boxed operand stack slots
            Gp bBase = cc.new_gp64();
            cc.mov(bBase, boxedBase);
            Gp bCount = cc.new_gp64();
            cc.mov(bCount, static_cast<int64_t>(MAX_OP_STACK));
            InvokeNode* cleanBoxed;
            cc.invoke(Out(cleanBoxed), reinterpret_cast<uint64_t>(jit_locals_cleanup),
                      FuncSignature::build<void, value::Value*, size_t>());
            cleanBoxed->set_arg(0, bBase);
            cleanBoxed->set_arg(1, bCount);
            // Clean up all locals
            Gp lBase = cc.new_gp64();
            cc.mov(lBase, localsBase);
            Gp lCount = cc.new_gp64();
            cc.mov(lCount, static_cast<int64_t>(localCount));
            InvokeNode* cleanLocals;
            cc.invoke(Out(cleanLocals), reinterpret_cast<uint64_t>(jit_locals_cleanup),
                      FuncSignature::build<void, value::Value*, size_t>());
            cleanLocals->set_arg(0, lBase);
            cleanLocals->set_arg(1, lCount);
        };

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
                {
                    SlotType pt = popType();
                    stackDepth--;
                    if (usesBoxedTypes && isBoxedSlotType(pt))
                    {
                        Gp addr = cc.new_gp64();
                        cc.lea(addr, Mem(boxedBase, static_cast<int32_t>(stackDepth * valueSize)));
                        InvokeNode* inv;
                        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_destroy),
                                  FuncSignature::build<void, value::Value*>());
                        inv->set_arg(0, addr);
                    }
                    break;
                }

                case OpCode::DUP:
                {
                    SlotType tt = topType();
                    if (usesBoxedTypes && isBoxedSlotType(tt))
                    {
                        Gp src = cc.new_gp64();
                        cc.lea(src, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));
                        Gp dst = cc.new_gp64();
                        cc.lea(dst, Mem(boxedBase, static_cast<int32_t>(stackDepth * valueSize)));
                        InvokeNode* inv;
                        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                                  FuncSignature::build<void, value::Value*, const value::Value*>());
                        inv->set_arg(0, dst);
                        inv->set_arg(1, src);
                    }
                    else
                    {
                        Gp tmp = cc.new_gp64();
                        cc.mov(tmp, Mem(stackBase, (stackDepth - 1) * 8));
                        cc.mov(Mem(stackBase, stackDepth * 8), tmp);
                    }
                    slotTypes.push_back(tt);
                    stackDepth++;
                    break;
                }

                case OpCode::SWAP:
                {
                    SlotType t1 = slotTypes.size() >= 1 ? slotTypes[slotTypes.size() - 1] : SlotType::INT;
                    SlotType t2 = slotTypes.size() >= 2 ? slotTypes[slotTypes.size() - 2] : SlotType::INT;
                    bool b1 = isBoxedSlotType(t1), b2 = isBoxedSlotType(t2);

                    if (usesBoxedTypes && (b1 || b2))
                    {
                        if (b1 && b2)
                        {
                            Gp addrA = cc.new_gp64();
                            cc.lea(addrA, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));
                            Gp addrB = cc.new_gp64();
                            cc.lea(addrB, Mem(boxedBase, static_cast<int32_t>((stackDepth - 2) * valueSize)));
                            InvokeNode* inv;
                            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_swap),
                                      FuncSignature::build<void, value::Value*, value::Value*>());
                            inv->set_arg(0, addrA);
                            inv->set_arg(1, addrB);
                        }
                        else
                        {
                            // Mixed boxed/primitive swap - bail out
                            compileFailed = true;
                            break;
                        }
                    }
                    else
                    {
                        Gp a = cc.new_gp64();
                        Gp b = cc.new_gp64();
                        cc.mov(a, Mem(stackBase, (stackDepth - 1) * 8));
                        cc.mov(b, Mem(stackBase, (stackDepth - 2) * 8));
                        cc.mov(Mem(stackBase, (stackDepth - 1) * 8), b);
                        cc.mov(Mem(stackBase, (stackDepth - 2) * 8), a);
                    }
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
                    SlotType lt = localTypes.count(slot) ? localTypes[slot] : SlotType::INT;

                    if (usesBoxedTypes && isBoxedSlotType(lt))
                    {
                        // Boxed local: copy Value from local to boxed stack
                        Gp src = cc.new_gp64();
                        cc.lea(src, Mem(localsBase, static_cast<int32_t>(slot * localStride)));
                        Gp dst = cc.new_gp64();
                        cc.lea(dst, Mem(boxedBase, static_cast<int32_t>(stackDepth * valueSize)));
                        InvokeNode* inv;
                        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                                  FuncSignature::build<void, value::Value*, const value::Value*>());
                        inv->set_arg(0, dst);
                        inv->set_arg(1, src);
                    }
                    else if (usesBoxedTypes)
                    {
                        // Primitive in boxed mode: unbox from Value-sized local
                        Gp localAddr = cc.new_gp64();
                        cc.lea(localAddr, Mem(localsBase, static_cast<int32_t>(slot * localStride)));
                        if (lt == SlotType::FLOAT)
                        {
                            InvokeNode* inv;
                            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_float),
                                      FuncSignature::build<float, const value::Value*>());
                            inv->set_arg(0, localAddr);
                            Vec val = cc.new_xmm();
                            inv->set_ret(0, val);
                            cc.movss(Mem(stackBase, stackDepth * 8), val);
                        }
                        else
                        {
                            InvokeNode* inv;
                            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_int),
                                      FuncSignature::build<int64_t, const value::Value*>());
                            inv->set_arg(0, localAddr);
                            Gp val = cc.new_gp64();
                            inv->set_ret(0, val);
                            cc.mov(Mem(stackBase, stackDepth * 8), val);
                        }
                    }
                    else
                    {
                        // Phase 3 path: 8-byte locals
                        Gp tmp = cc.new_gp64();
                        cc.mov(tmp, Mem(localsBase, static_cast<int32_t>(slot * 8)));
                        cc.mov(Mem(stackBase, stackDepth * 8), tmp);
                    }
                    slotTypes.push_back(lt);
                    stackDepth++;
                    break;
                }

                case OpCode::STORE_LOCAL:
                {
                    size_t slot = instr.operands[0];
                    SlotType tt = topType();

                    if (usesBoxedTypes && isBoxedSlotType(tt))
                    {
                        // Store boxed value from boxed stack to local
                        Gp src = cc.new_gp64();
                        cc.lea(src, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));
                        Gp dst = cc.new_gp64();
                        cc.lea(dst, Mem(localsBase, static_cast<int32_t>(slot * localStride)));
                        InvokeNode* inv;
                        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                                  FuncSignature::build<void, value::Value*, const value::Value*>());
                        inv->set_arg(0, dst);
                        inv->set_arg(1, src);
                    }
                    else if (usesBoxedTypes)
                    {
                        // Primitive in boxed mode: box into Value-sized local
                        Gp destAddr = cc.new_gp64();
                        cc.lea(destAddr, Mem(localsBase, static_cast<int32_t>(slot * localStride)));
                        if (tt == SlotType::FLOAT)
                        {
                            Vec val = cc.new_xmm();
                            cc.movss(val, Mem(stackBase, (stackDepth - 1) * 8));
                            InvokeNode* inv;
                            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_box_float),
                                      FuncSignature::build<void, value::Value*, float>());
                            inv->set_arg(0, destAddr);
                            inv->set_arg(1, val);
                        }
                        else
                        {
                            Gp val = cc.new_gp64();
                            cc.mov(val, Mem(stackBase, (stackDepth - 1) * 8));
                            uint64_t fn = (tt == SlotType::BOOL)
                                ? reinterpret_cast<uint64_t>(jit_box_bool)
                                : reinterpret_cast<uint64_t>(jit_box_int);
                            InvokeNode* inv;
                            cc.invoke(Out(inv), fn,
                                      FuncSignature::build<void, value::Value*, int64_t>());
                            inv->set_arg(0, destAddr);
                            inv->set_arg(1, val);
                        }
                    }
                    else
                    {
                        // Phase 3 path
                        Gp tmp = cc.new_gp64();
                        cc.mov(tmp, Mem(stackBase, (stackDepth - 1) * 8));
                        cc.mov(Mem(localsBase, static_cast<int32_t>(slot * 8)), tmp);
                    }
                    localTypes[slot] = tt;
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
                    emitCleanup();
                    cc.mov(Mem(ctxPtr, offsetof(JitContext, hasReturnValue)), 0);
                    cc.ret();
                    break;

                case OpCode::RETURN_VALUE:
                {
                    stackDepth--;
                    SlotType retType = popType();

                    if (usesBoxedTypes && isBoxedSlotType(retType))
                    {
                        // Boxed return value
                        Gp retAddr = cc.new_gp64();
                        cc.lea(retAddr, Mem(boxedBase, static_cast<int32_t>(stackDepth * valueSize)));
                        InvokeNode* inv;
                        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_boxed),
                                  FuncSignature::build<void, JitContext*, const value::Value*>());
                        inv->set_arg(0, ctxPtr);
                        inv->set_arg(1, retAddr);
                    }
                    else if (retType == SlotType::FLOAT)
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
                    emitCleanup();
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

                    bool isPrimReturn = (returnType == "int" || returnType == "float" ||
                                         returnType == "bool" || returnType == "void");
                    if (!isPrimReturn && !usesBoxedTypes)
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

                        if (usesBoxedTypes && isBoxedSlotType(argType))
                        {
                            // Copy boxed value to callArgs
                            Gp srcAddr = cc.new_gp64();
                            cc.lea(srcAddr, Mem(boxedBase, static_cast<int32_t>(stackIdx * valueSize)));
                            InvokeNode* cpInv;
                            cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                                      FuncSignature::build<void, value::Value*, const value::Value*>());
                            cpInv->set_arg(0, destAddr);
                            cpInv->set_arg(1, srcAddr);
                        }
                        else
                        {
                            emitBox(destAddr, stackIdx, argType);
                        }
                    }

                    // Pop arguments (destroy boxed slots)
                    for (size_t i = 0; i < argCount; ++i)
                    {
                        SlotType at = popType();
                        if (usesBoxedTypes && isBoxedSlotType(at))
                        {
                            int idx = stackDepth - static_cast<int>(argCount) + static_cast<int>(i);
                            Gp addr = cc.new_gp64();
                            cc.lea(addr, Mem(boxedBase, static_cast<int32_t>(idx * valueSize)));
                            InvokeNode* dInv;
                            cc.invoke(Out(dInv), reinterpret_cast<uint64_t>(jit_value_destroy),
                                      FuncSignature::build<void, value::Value*>());
                            dInv->set_arg(0, addr);
                        }
                    }
                    stackDepth -= static_cast<int>(argCount);

                    // Call jit_call_function(ctx, nameIndex, argCount)
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

                    // Handle return value
                    if (returnType != "void")
                    {
                        if (isPrimReturn)
                        {
                            SlotType retSlot = SlotType::INT;
                            if (returnType == "float") retSlot = SlotType::FLOAT;
                            else if (returnType == "bool") retSlot = SlotType::BOOL;

                            Gp retAddr = cc.new_gp64();
                            cc.lea(retAddr, Mem(ctxPtr, offsetof(JitContext, returnValue)));
                            emitUnbox(retAddr, stackDepth, retSlot);
                            slotTypes.push_back(retSlot);
                        }
                        else
                        {
                            // Non-primitive return: copy to boxed stack
                            Gp retAddr = cc.new_gp64();
                            cc.lea(retAddr, Mem(ctxPtr, offsetof(JitContext, returnValue)));
                            Gp destAddr = cc.new_gp64();
                            cc.lea(destAddr, Mem(boxedBase, static_cast<int32_t>(stackDepth * valueSize)));
                            InvokeNode* cpInv;
                            cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                                      FuncSignature::build<void, value::Value*, const value::Value*>());
                            cpInv->set_arg(0, destAddr);
                            cpInv->set_arg(1, retAddr);
                            slotTypes.push_back(SlotType::BOXED);
                        }
                        stackDepth++;
                    }
                    break;
                }

                // ===== Phase 4: String, Object, Array, Method, Type Operations =====
                case OpCode::PUSH_STRING:
                {
                    uint32_t constIndex = static_cast<uint32_t>(instr.operands[0]);
                    Gp dest = cc.new_gp64();
                    cc.lea(dest, Mem(boxedBase, static_cast<int32_t>(stackDepth * valueSize)));
                    Gp pPtr = cc.new_gp64();
                    cc.mov(pPtr, progPtr);
                    Gp idx = cc.new_gp64();
                    cc.mov(idx, static_cast<int64_t>(constIndex));
                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_push_string),
                              FuncSignature::build<void, value::Value*,
                                  const vm::bytecode::BytecodeProgram*, uint32_t>());
                    inv->set_arg(0, dest);
                    inv->set_arg(1, pPtr);
                    inv->set_arg(2, idx);
                    slotTypes.push_back(SlotType::STRING);
                    stackDepth++;
                    break;
                }

                case OpCode::GET_FIELD:
                {
                    uint32_t fieldNameIndex = static_cast<uint32_t>(instr.operands[0]);
                    // Object is on boxed stack at stackDepth-1
                    Gp objAddr = cc.new_gp64();
                    cc.lea(objAddr, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));
                    // Dest overwrites the object slot
                    Gp dest = cc.new_gp64();
                    cc.lea(dest, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));
                    Gp pPtr = cc.new_gp64();
                    cc.mov(pPtr, progPtr);
                    Gp idx = cc.new_gp64();
                    cc.mov(idx, static_cast<int64_t>(fieldNameIndex));
                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_get_field),
                              FuncSignature::build<void, value::Value*, const value::Value*,
                                  const vm::bytecode::BytecodeProgram*, uint32_t>());
                    inv->set_arg(0, dest);
                    inv->set_arg(1, objAddr);
                    inv->set_arg(2, pPtr);
                    inv->set_arg(3, idx);
                    popType(); // pop object
                    slotTypes.push_back(SlotType::BOXED);
                    // stackDepth unchanged (pop 1, push 1)
                    break;
                }

                case OpCode::SET_FIELD:
                {
                    uint32_t fieldNameIndex = static_cast<uint32_t>(instr.operands[0]);
                    SlotType valType = popType(); // value at stackDepth-1
                    popType(); // object at stackDepth-2

                    // If value is primitive, box it into a temp callArgs slot
                    Gp newValAddr = cc.new_gp64();
                    if (!isBoxedSlotType(valType))
                    {
                        cc.lea(newValAddr, Mem(ctxPtr, offsetof(JitContext, callArgs)));
                        emitBox(newValAddr, stackDepth - 1, valType);
                    }
                    else
                    {
                        cc.lea(newValAddr, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));
                    }

                    Gp objAddr = cc.new_gp64();
                    cc.lea(objAddr, Mem(boxedBase, static_cast<int32_t>((stackDepth - 2) * valueSize)));
                    // dest overwrites the object slot (stackDepth-2)
                    Gp dest = cc.new_gp64();
                    cc.lea(dest, Mem(boxedBase, static_cast<int32_t>((stackDepth - 2) * valueSize)));
                    Gp pPtr = cc.new_gp64();
                    cc.mov(pPtr, progPtr);
                    Gp idx = cc.new_gp64();
                    cc.mov(idx, static_cast<int64_t>(fieldNameIndex));

                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_field),
                              FuncSignature::build<void, value::Value*, const value::Value*,
                                  const value::Value*,
                                  const vm::bytecode::BytecodeProgram*, uint32_t>());
                    inv->set_arg(0, dest);
                    inv->set_arg(1, objAddr);
                    inv->set_arg(2, newValAddr);
                    inv->set_arg(3, pPtr);
                    inv->set_arg(4, idx);

                    // Destroy old value slot if boxed
                    if (isBoxedSlotType(valType))
                    {
                        Gp vAddr = cc.new_gp64();
                        cc.lea(vAddr, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));
                        InvokeNode* dInv;
                        cc.invoke(Out(dInv), reinterpret_cast<uint64_t>(jit_value_destroy),
                                  FuncSignature::build<void, value::Value*>());
                        dInv->set_arg(0, vAddr);
                    }

                    stackDepth--; // pop 2, push 1 = net -1
                    slotTypes.push_back(SlotType::BOXED);
                    break;
                }

                case OpCode::NEW_ARRAY:
                {
                    uint32_t typeIndex = static_cast<uint32_t>(instr.operands[0]);
                    popType(); // pop size (INT)

                    // Read size from primStack
                    Gp sizeVal = cc.new_gp64();
                    cc.mov(sizeVal, Mem(stackBase, (stackDepth - 1) * 8));

                    Gp dest = cc.new_gp64();
                    cc.lea(dest, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));
                    Gp idx = cc.new_gp64();
                    cc.mov(idx, static_cast<int64_t>(typeIndex));

                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_new_array),
                              FuncSignature::build<void, value::Value*, JitContext*,
                                  uint32_t, int64_t>());
                    inv->set_arg(0, dest);
                    inv->set_arg(1, ctxPtr);
                    inv->set_arg(2, idx);
                    inv->set_arg(3, sizeVal);

                    // stackDepth unchanged (pop 1 INT, push 1 ARRAY)
                    slotTypes.push_back(SlotType::ARRAY);
                    break;
                }

                case OpCode::ARRAY_GET:
                {
                    popType(); // pop index (INT)
                    popType(); // pop array (ARRAY/BOXED)

                    // index from primStack at stackDepth-1
                    Gp indexVal = cc.new_gp64();
                    cc.mov(indexVal, Mem(stackBase, (stackDepth - 1) * 8));
                    // array from boxedStack at stackDepth-2
                    Gp arrAddr = cc.new_gp64();
                    cc.lea(arrAddr, Mem(boxedBase, static_cast<int32_t>((stackDepth - 2) * valueSize)));
                    // dest overwrites array slot
                    Gp dest = cc.new_gp64();
                    cc.lea(dest, Mem(boxedBase, static_cast<int32_t>((stackDepth - 2) * valueSize)));

                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_array_get),
                              FuncSignature::build<void, value::Value*, const value::Value*,
                                  int64_t>());
                    inv->set_arg(0, dest);
                    inv->set_arg(1, arrAddr);
                    inv->set_arg(2, indexVal);

                    stackDepth--; // pop 2, push 1
                    slotTypes.push_back(SlotType::BOXED);
                    break;
                }

                case OpCode::ARRAY_SET:
                {
                    SlotType valType = popType(); // value
                    popType(); // index (INT)
                    popType(); // array (ARRAY/BOXED)

                    // Box value if primitive
                    Gp valAddr = cc.new_gp64();
                    if (!isBoxedSlotType(valType))
                    {
                        cc.lea(valAddr, Mem(ctxPtr, offsetof(JitContext, callArgs)));
                        emitBox(valAddr, stackDepth - 1, valType);
                    }
                    else
                    {
                        cc.lea(valAddr, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));
                    }

                    Gp indexVal = cc.new_gp64();
                    cc.mov(indexVal, Mem(stackBase, (stackDepth - 2) * 8));
                    Gp arrAddr = cc.new_gp64();
                    cc.lea(arrAddr, Mem(boxedBase, static_cast<int32_t>((stackDepth - 3) * valueSize)));

                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_array_set),
                              FuncSignature::build<void, const value::Value*, int64_t,
                                  const value::Value*>());
                    inv->set_arg(0, arrAddr);
                    inv->set_arg(1, indexVal);
                    inv->set_arg(2, valAddr);

                    // Destroy boxed slots
                    if (isBoxedSlotType(valType))
                    {
                        Gp dAddr = cc.new_gp64();
                        cc.lea(dAddr, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));
                        InvokeNode* dInv;
                        cc.invoke(Out(dInv), reinterpret_cast<uint64_t>(jit_value_destroy),
                                  FuncSignature::build<void, value::Value*>());
                        dInv->set_arg(0, dAddr);
                    }
                    {
                        Gp dAddr = cc.new_gp64();
                        cc.lea(dAddr, Mem(boxedBase, static_cast<int32_t>((stackDepth - 3) * valueSize)));
                        InvokeNode* dInv;
                        cc.invoke(Out(dInv), reinterpret_cast<uint64_t>(jit_value_destroy),
                                  FuncSignature::build<void, value::Value*>());
                        dInv->set_arg(0, dAddr);
                    }

                    stackDepth -= 3; // pop all 3, push nothing
                    break;
                }

                case OpCode::ARRAY_LENGTH:
                {
                    popType(); // pop array (ARRAY/BOXED)

                    Gp arrAddr = cc.new_gp64();
                    cc.lea(arrAddr, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));

                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_array_length),
                              FuncSignature::build<int64_t, const value::Value*>());
                    inv->set_arg(0, arrAddr);
                    Gp lenVal = cc.new_gp64();
                    inv->set_ret(0, lenVal);

                    // Destroy the array boxed slot
                    {
                        Gp dAddr = cc.new_gp64();
                        cc.lea(dAddr, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));
                        InvokeNode* dInv;
                        cc.invoke(Out(dInv), reinterpret_cast<uint64_t>(jit_value_destroy),
                                  FuncSignature::build<void, value::Value*>());
                        dInv->set_arg(0, dAddr);
                    }

                    // Push length as INT on primStack
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), lenVal);
                    slotTypes.push_back(SlotType::INT);
                    // stackDepth unchanged (pop 1, push 1)
                    break;
                }

                case OpCode::CALL_STATIC:
                {
                    uint32_t nameIndex = static_cast<uint32_t>(instr.operands[0]);
                    size_t argCount = instr.operands[1];

                    if (argCount > JitContext::MAX_CALL_ARGS)
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
                        if (isBoxedSlotType(argType))
                        {
                            Gp srcAddr = cc.new_gp64();
                            cc.lea(srcAddr, Mem(boxedBase, static_cast<int32_t>(stackIdx * valueSize)));
                            InvokeNode* cpInv;
                            cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                                      FuncSignature::build<void, value::Value*, const value::Value*>());
                            cpInv->set_arg(0, destAddr);
                            cpInv->set_arg(1, srcAddr);
                        }
                        else
                        {
                            emitBox(destAddr, stackIdx, argType);
                        }
                    }

                    // Pop and destroy boxed args
                    for (size_t i = 0; i < argCount; ++i)
                    {
                        SlotType at = popType();
                        if (isBoxedSlotType(at))
                        {
                            int idx = stackDepth - static_cast<int>(argCount) + static_cast<int>(i);
                            Gp addr = cc.new_gp64();
                            cc.lea(addr, Mem(boxedBase, static_cast<int32_t>(idx * valueSize)));
                            InvokeNode* dInv;
                            cc.invoke(Out(dInv), reinterpret_cast<uint64_t>(jit_value_destroy),
                                      FuncSignature::build<void, value::Value*>());
                            dInv->set_arg(0, addr);
                        }
                    }
                    stackDepth -= static_cast<int>(argCount);

                    Gp niReg = cc.new_gp64();
                    cc.mov(niReg, static_cast<int64_t>(nameIndex));
                    Gp acReg = cc.new_gp64();
                    cc.mov(acReg, static_cast<int64_t>(argCount));

                    InvokeNode* callInv;
                    cc.invoke(Out(callInv), reinterpret_cast<uint64_t>(jit_call_static),
                              FuncSignature::build<void, JitContext*, uint32_t, size_t>());
                    callInv->set_arg(0, ctxPtr);
                    callInv->set_arg(1, niReg);
                    callInv->set_arg(2, acReg);

                    // Push return as BOXED (we don't know static method return type easily)
                    {
                        Gp retAddr = cc.new_gp64();
                        cc.lea(retAddr, Mem(ctxPtr, offsetof(JitContext, returnValue)));
                        Gp destAddr = cc.new_gp64();
                        cc.lea(destAddr, Mem(boxedBase, static_cast<int32_t>(stackDepth * valueSize)));
                        InvokeNode* cpInv;
                        cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                                  FuncSignature::build<void, value::Value*, const value::Value*>());
                        cpInv->set_arg(0, destAddr);
                        cpInv->set_arg(1, retAddr);
                        slotTypes.push_back(SlotType::BOXED);
                        stackDepth++;
                    }
                    break;
                }

                case OpCode::CALL_METHOD:
                {
                    uint32_t methodNameIndex = static_cast<uint32_t>(instr.operands[0]);
                    size_t argCount = instr.operands[1];

                    // Stack: [object, arg1, ..., argN]
                    // callArgs[0] = object, callArgs[1..N] = args
                    if (argCount + 1 > JitContext::MAX_CALL_ARGS)
                    {
                        compileFailed = true;
                        break;
                    }

                    // Box object into callArgs[0]
                    {
                        int objIdx = stackDepth - static_cast<int>(argCount) - 1;
                        Gp destAddr = cc.new_gp64();
                        cc.lea(destAddr, Mem(ctxPtr, offsetof(JitContext, callArgs)));
                        Gp srcAddr = cc.new_gp64();
                        cc.lea(srcAddr, Mem(boxedBase, static_cast<int32_t>(objIdx * valueSize)));
                        InvokeNode* cpInv;
                        cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                                  FuncSignature::build<void, value::Value*, const value::Value*>());
                        cpInv->set_arg(0, destAddr);
                        cpInv->set_arg(1, srcAddr);
                    }

                    // Box args into callArgs[1..argCount]
                    for (size_t i = 0; i < argCount; ++i)
                    {
                        int stackIdx = stackDepth - static_cast<int>(argCount) + static_cast<int>(i);
                        SlotType argType = (stackIdx >= 0 && stackIdx < static_cast<int>(slotTypes.size()))
                                         ? slotTypes[stackIdx] : SlotType::INT;
                        Gp destAddr = cc.new_gp64();
                        cc.lea(destAddr, Mem(ctxPtr, offsetof(JitContext, callArgs)
                                              + static_cast<int32_t>((i + 1) * valueSize)));
                        if (isBoxedSlotType(argType))
                        {
                            Gp srcAddr = cc.new_gp64();
                            cc.lea(srcAddr, Mem(boxedBase, static_cast<int32_t>(stackIdx * valueSize)));
                            InvokeNode* cpInv;
                            cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                                      FuncSignature::build<void, value::Value*, const value::Value*>());
                            cpInv->set_arg(0, destAddr);
                            cpInv->set_arg(1, srcAddr);
                        }
                        else
                        {
                            emitBox(destAddr, stackIdx, argType);
                        }
                    }

                    // Pop args + object, destroy boxed slots
                    for (size_t i = 0; i < argCount; ++i)
                    {
                        SlotType at = popType();
                        if (isBoxedSlotType(at))
                        {
                            int idx = stackDepth - static_cast<int>(argCount) + static_cast<int>(i) - 1;
                            // +1 offset for object at bottom
                            Gp addr = cc.new_gp64();
                            cc.lea(addr, Mem(boxedBase, static_cast<int32_t>((idx + 1) * valueSize)));
                            InvokeNode* dInv;
                            cc.invoke(Out(dInv), reinterpret_cast<uint64_t>(jit_value_destroy),
                                      FuncSignature::build<void, value::Value*>());
                            dInv->set_arg(0, addr);
                        }
                    }
                    popType(); // pop object type
                    {
                        int objIdx = stackDepth - static_cast<int>(argCount) - 1;
                        Gp addr = cc.new_gp64();
                        cc.lea(addr, Mem(boxedBase, static_cast<int32_t>(objIdx * valueSize)));
                        InvokeNode* dInv;
                        cc.invoke(Out(dInv), reinterpret_cast<uint64_t>(jit_value_destroy),
                                  FuncSignature::build<void, value::Value*>());
                        dInv->set_arg(0, addr);
                    }
                    stackDepth -= static_cast<int>(argCount) + 1;

                    Gp miReg = cc.new_gp64();
                    cc.mov(miReg, static_cast<int64_t>(methodNameIndex));
                    Gp acReg = cc.new_gp64();
                    cc.mov(acReg, static_cast<int64_t>(argCount));

                    InvokeNode* callInv;
                    cc.invoke(Out(callInv), reinterpret_cast<uint64_t>(jit_call_method),
                              FuncSignature::build<void, JitContext*, uint32_t, size_t>());
                    callInv->set_arg(0, ctxPtr);
                    callInv->set_arg(1, miReg);
                    callInv->set_arg(2, acReg);

                    // Push return value as BOXED
                    {
                        Gp retAddr = cc.new_gp64();
                        cc.lea(retAddr, Mem(ctxPtr, offsetof(JitContext, returnValue)));
                        Gp destAddr = cc.new_gp64();
                        cc.lea(destAddr, Mem(boxedBase, static_cast<int32_t>(stackDepth * valueSize)));
                        InvokeNode* cpInv;
                        cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                                  FuncSignature::build<void, value::Value*, const value::Value*>());
                        cpInv->set_arg(0, destAddr);
                        cpInv->set_arg(1, retAddr);
                        slotTypes.push_back(SlotType::BOXED);
                        stackDepth++;
                    }
                    break;
                }

                case OpCode::INSTANCEOF:
                {
                    uint32_t typeIndex = static_cast<uint32_t>(instr.operands[0]);
                    SlotType vType = popType();

                    // Box value if primitive, otherwise use boxed stack
                    Gp valAddr = cc.new_gp64();
                    if (!isBoxedSlotType(vType))
                    {
                        cc.lea(valAddr, Mem(ctxPtr, offsetof(JitContext, callArgs)));
                        emitBox(valAddr, stackDepth - 1, vType);
                    }
                    else
                    {
                        cc.lea(valAddr, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));
                    }

                    Gp pPtr = cc.new_gp64();
                    cc.mov(pPtr, progPtr);
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

                    // Destroy boxed slot if needed
                    if (isBoxedSlotType(vType))
                    {
                        Gp dAddr = cc.new_gp64();
                        cc.lea(dAddr, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));
                        InvokeNode* dInv;
                        cc.invoke(Out(dInv), reinterpret_cast<uint64_t>(jit_value_destroy),
                                  FuncSignature::build<void, value::Value*>());
                        dInv->set_arg(0, dAddr);
                    }

                    // Push result as BOOL on primStack
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), result);
                    slotTypes.push_back(SlotType::BOOL);
                    // stackDepth unchanged (pop 1, push 1)
                    break;
                }

                case OpCode::CAST:
                {
                    uint32_t typeIndex = static_cast<uint32_t>(instr.operands[0]);
                    SlotType srcType = popType();

                    // Box source if primitive
                    Gp srcAddr = cc.new_gp64();
                    if (!isBoxedSlotType(srcType))
                    {
                        cc.lea(srcAddr, Mem(ctxPtr, offsetof(JitContext, callArgs)));
                        emitBox(srcAddr, stackDepth - 1, srcType);
                    }
                    else
                    {
                        cc.lea(srcAddr, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));
                    }

                    // Dest in boxed stack (overwrite)
                    Gp dest = cc.new_gp64();
                    cc.lea(dest, Mem(boxedBase, static_cast<int32_t>((stackDepth - 1) * valueSize)));
                    Gp pPtr = cc.new_gp64();
                    cc.mov(pPtr, progPtr);
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

                    // Result is always BOXED (cast could produce any type)
                    slotTypes.push_back(SlotType::BOXED);
                    // stackDepth unchanged
                    break;
                }

                case OpCode::NEW_OBJECT:
                {
                    uint32_t classIndex = static_cast<uint32_t>(instr.operands[0]);
                    size_t argCount = instr.operands[1];

                    if (argCount > JitContext::MAX_CALL_ARGS)
                    {
                        compileFailed = true;
                        break;
                    }

                    // Box constructor args into ctx->callArgs[]
                    for (size_t i = 0; i < argCount; ++i)
                    {
                        int stackIdx = stackDepth - static_cast<int>(argCount) + static_cast<int>(i);
                        SlotType argType = (stackIdx >= 0 && stackIdx < static_cast<int>(slotTypes.size()))
                                         ? slotTypes[stackIdx] : SlotType::INT;
                        Gp destAddr = cc.new_gp64();
                        cc.lea(destAddr, Mem(ctxPtr, offsetof(JitContext, callArgs)
                                              + static_cast<int32_t>(i * valueSize)));
                        if (isBoxedSlotType(argType))
                        {
                            Gp srcAddr = cc.new_gp64();
                            cc.lea(srcAddr, Mem(boxedBase, static_cast<int32_t>(stackIdx * valueSize)));
                            InvokeNode* cpInv;
                            cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                                      FuncSignature::build<void, value::Value*, const value::Value*>());
                            cpInv->set_arg(0, destAddr);
                            cpInv->set_arg(1, srcAddr);
                        }
                        else
                        {
                            emitBox(destAddr, stackIdx, argType);
                        }
                    }

                    // Pop and destroy boxed args
                    for (size_t i = 0; i < argCount; ++i)
                    {
                        SlotType at = popType();
                        if (isBoxedSlotType(at))
                        {
                            int idx = stackDepth - static_cast<int>(argCount) + static_cast<int>(i);
                            Gp addr = cc.new_gp64();
                            cc.lea(addr, Mem(boxedBase, static_cast<int32_t>(idx * valueSize)));
                            InvokeNode* dInv;
                            cc.invoke(Out(dInv), reinterpret_cast<uint64_t>(jit_value_destroy),
                                      FuncSignature::build<void, value::Value*>());
                            dInv->set_arg(0, addr);
                        }
                    }
                    stackDepth -= static_cast<int>(argCount);

                    // Call jit_new_object(dest, ctx, classIndex, argCount)
                    Gp dest = cc.new_gp64();
                    cc.lea(dest, Mem(boxedBase, static_cast<int32_t>(stackDepth * valueSize)));
                    Gp ciReg = cc.new_gp64();
                    cc.mov(ciReg, static_cast<int64_t>(classIndex));
                    Gp acReg = cc.new_gp64();
                    cc.mov(acReg, static_cast<int64_t>(argCount));

                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_new_object),
                              FuncSignature::build<void, value::Value*, JitContext*,
                                  uint32_t, size_t>());
                    inv->set_arg(0, dest);
                    inv->set_arg(1, ctxPtr);
                    inv->set_arg(2, ciReg);
                    inv->set_arg(3, acReg);

                    slotTypes.push_back(SlotType::OBJECT);
                    stackDepth++;
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

        emitCleanup();
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
