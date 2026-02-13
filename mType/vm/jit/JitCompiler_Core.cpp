#include "JitCompiler.hpp"
#include "JitEmissionState.hpp"
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

    bool JitCompiler::canCompileLoopOSR(size_t loopStartOffset, size_t loopEndOffset,
                                         const bytecode::BytecodeProgram& program) const
    {
        const auto& supported = getSupportedOpcodes();
        for (size_t ip = loopStartOffset; ip <= loopEndOffset; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            if (supported.find(static_cast<uint8_t>(instr.opcode)) == supported.end())
                return false;
        }
        return true;
    }

    // ===== Shared emission helpers =====

    void emitBox(JitEmissionState& s, Gp destAddr, int stackOff, SlotType type)
    {
        auto& cc = s.cc;
        if (type == SlotType::FLOAT)
        {
            Vec val = cc.new_xmm();
            cc.movss(val, Mem(s.stackBase, stackOff * 8));
            InvokeNode* inv;
            cc.invoke(Out(inv), (uint64_t)jit_box_float,
                      FuncSignature::build<void, value::Value*, float>());
            inv->set_arg(0, destAddr);
            inv->set_arg(1, val);
        }
        else
        {
            Gp val = cc.new_gp64();
            cc.mov(val, Mem(s.stackBase, stackOff * 8));
            uint64_t fn = (type == SlotType::BOOL) ? (uint64_t)jit_box_bool : (uint64_t)jit_box_int;
            InvokeNode* inv;
            cc.invoke(Out(inv), fn, FuncSignature::build<void, value::Value*, int64_t>());
            inv->set_arg(0, destAddr);
            inv->set_arg(1, val);
        }
    }

    void emitUnbox(JitEmissionState& s, Gp srcAddr, int stackOff, SlotType type)
    {
        auto& cc = s.cc;
        if (type == SlotType::FLOAT)
        {
            InvokeNode* inv;
            cc.invoke(Out(inv), (uint64_t)jit_unbox_float,
                      FuncSignature::build<float, const value::Value*>());
            inv->set_arg(0, srcAddr);
            Vec val = cc.new_xmm();
            inv->set_ret(0, val);
            cc.movss(Mem(s.stackBase, stackOff * 8), val);
        }
        else
        {
            InvokeNode* inv;
            cc.invoke(Out(inv), (uint64_t)jit_unbox_int,
                      FuncSignature::build<int64_t, const value::Value*>());
            inv->set_arg(0, srcAddr);
            Gp val = cc.new_gp64();
            inv->set_ret(0, val);
            cc.mov(Mem(s.stackBase, stackOff * 8), val);
        }
    }

    SlotType popType(JitEmissionState& s)
    {
        if (s.slotTypes.empty()) return SlotType::INT;
        SlotType t = s.slotTypes.back();
        s.slotTypes.pop_back();
        return t;
    }

    SlotType topType(const JitEmissionState& s)
    {
        return s.slotTypes.empty() ? SlotType::INT : s.slotTypes.back();
    }

    void emitCleanup(JitEmissionState& s)
    {
        if (!s.usesBoxedTypes) return;
        auto& cc = s.cc;
        Gp bBase = cc.new_gp64();
        cc.mov(bBase, s.boxedBase);
        Gp bCount = cc.new_gp64();
        cc.mov(bCount, static_cast<int64_t>(JitEmissionState::MAX_OP_STACK));
        InvokeNode* cleanBoxed;
        cc.invoke(Out(cleanBoxed), reinterpret_cast<uint64_t>(jit_locals_cleanup),
                  FuncSignature::build<void, value::Value*, size_t>());
        cleanBoxed->set_arg(0, bBase);
        cleanBoxed->set_arg(1, bCount);
        Gp lBase = cc.new_gp64();
        cc.mov(lBase, s.localsBase);
        Gp lCount = cc.new_gp64();
        cc.mov(lCount, static_cast<int64_t>(s.localCount));
        InvokeNode* cleanLocals;
        cc.invoke(Out(cleanLocals), reinterpret_cast<uint64_t>(jit_locals_cleanup),
                  FuncSignature::build<void, value::Value*, size_t>());
        cleanLocals->set_arg(0, lBase);
        cleanLocals->set_arg(1, lCount);
    }

    void emitGenericBinop(JitEmissionState& s, uint64_t helperFn,
                          SlotType lType, SlotType rType)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        Gp leftAddr = cc.new_gp64();
        cc.lea(leftAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)));
        emitBox(s, leftAddr, s.stackDepth - 1, lType);

        Gp rightAddr = cc.new_gp64();
        cc.lea(rightAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)
                              + static_cast<int32_t>(valueSize)));
        emitBox(s, rightAddr, s.stackDepth, rType);

        Gp resultAddr = cc.new_gp64();
        cc.lea(resultAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)
                                + static_cast<int32_t>(2 * valueSize)));

        InvokeNode* inv;
        cc.invoke(Out(inv), helperFn,
                  FuncSignature::build<void, value::Value*, const value::Value*, const value::Value*>());
        inv->set_arg(0, resultAddr);
        inv->set_arg(1, leftAddr);
        inv->set_arg(2, rightAddr);

        SlotType resType = (lType == SlotType::FLOAT || rType == SlotType::FLOAT)
                         ? SlotType::FLOAT : SlotType::INT;
        emitUnbox(s, resultAddr, s.stackDepth - 1, resType);
        s.slotTypes.push_back(resType);
    }

    void emitCmp(JitEmissionState& s, CmpOp kind)
    {
        auto& cc = s.cc;
        s.stackDepth--;
        SlotType rType = popType(s);
        SlotType lType = popType(s);

        Gp result = cc.new_gp64();
        cc.xor_(result, result);

        if (lType == SlotType::FLOAT || rType == SlotType::FLOAT)
        {
            Vec right = cc.new_xmm();
            Vec left = cc.new_xmm();
            cc.movss(right, Mem(s.stackBase, s.stackDepth * 8));
            cc.movss(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
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
            cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
            cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
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
        cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), result);
        s.slotTypes.push_back(SlotType::BOOL);
    }

    // ===== compile() =====

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
        constexpr size_t MAX_LOCAL_COUNT = 1024;
        if (localCount > MAX_LOCAL_COUNT)
        {
            bailoutCount++;
            return false;
        }
        constexpr size_t MAX_OP_STACK = 64;
        constexpr size_t valueSize = sizeof(value::Value);

        // Pre-scan: determine if function uses boxed types
        bool usesBoxedTypes = false;
        {
            size_t scanEnd = funcMeta->startOffset + funcMeta->instructionCount;
            for (size_t ip = funcMeta->startOffset; ip < scanEnd; ++ip)
            {
                const auto& si = program.getInstruction(ip);
                switch (si.opcode)
                {
                    case OpCode::PUSH_STRING: case OpCode::GET_FIELD:
                    case OpCode::SET_FIELD:   case OpCode::NEW_OBJECT:
                    case OpCode::CALL_METHOD: case OpCode::CALL_STATIC:
                    case OpCode::NEW_ARRAY:   case OpCode::ARRAY_GET:
                    case OpCode::ARRAY_SET:   case OpCode::ARRAY_LENGTH:
                    case OpCode::INSTANCEOF:  case OpCode::CAST:
                        usesBoxedTypes = true;
                        break;
                    default: break;
                }
                if (usesBoxedTypes) break;
            }
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

        Gp boxedBase = cc.new_gp64("boxedBase");
        if (usesBoxedTypes)
        {
            Mem boxedArea = cc.new_stack(static_cast<uint32_t>(MAX_OP_STACK * valueSize), 8);
            cc.lea(boxedBase, boxedArea);
        }

        Gp progPtr = cc.new_gp64("progPtr");
        if (usesBoxedTypes)
        {
            cc.mov(progPtr, reinterpret_cast<uint64_t>(&program));
        }

        std::unordered_map<size_t, SlotType> localTypes;

        // Initialize locals and boxed stacks
        if (usesBoxedTypes)
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
                    if (paramType == SlotType::FLOAT)
                    {
                        InvokeNode* inv;
                        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_float),
                                  FuncSignature::build<float, const value::Value*>());
                        inv->set_arg(0, argAddr);
                        Vec val = cc.new_xmm();
                        inv->set_ret(0, val);
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

        // Zero-init non-parameter locals
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

        // Build emission state and run main codegen loop
        JitEmissionState s{cc, ctxPtr, localsBase, stackBase, boxedBase, progPtr,
                           usesBoxedTypes, localCount, localStride,
                           0, {}, localTypes, false, labels, program};

        for (size_t ip = startOffset; ip < startOffset + instrCount && !s.compileFailed; ++ip)
        {
            auto labelIt = s.labels.find(ip);
            if (labelIt != s.labels.end())
                cc.bind(labelIt->second);

            const auto& instr = program.getInstruction(ip);
            if (emitCoreOps(s, instr)) continue;
            if (emitArithmeticOps(s, instr)) continue;
            if (emitControlFlowOps(s, instr, nullptr)) continue;
            emitObjectOps(s, instr);
        }

        if (s.compileFailed)
        {
            bailoutCount++;
            return false;
        }

        emitCleanup(s);
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
