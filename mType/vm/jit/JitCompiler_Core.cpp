#include "JitCompiler.hpp"
#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    JitCompiler::JitCompiler() {}

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

    void emitBox(JitEmissionState& s, Gp destAddr, int stackOff, SlotType type)
    {
        auto& cc = s.cc;
        if (type == SlotType::FLOAT)
        {
            Vec val = cc.new_xmm();
            cc.movsd(val, Mem(s.stackBase, stackOff * 8));
            InvokeNode* inv;
            cc.invoke(Out(inv), (uint64_t)jit_box_float,
                      FuncSignature::build<void, value::Value*, double>());
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
                      FuncSignature::build<double, const value::Value*>());
            inv->set_arg(0, srcAddr);
            Vec val = cc.new_xmm();
            inv->set_ret(0, val);
            cc.movsd(Mem(s.stackBase, stackOff * 8), val);
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

    void emitEnsureUnboxed(JitEmissionState& s, int stackIdx,
                           SlotType type, SlotType targetType)
    {
        if (!s.usesBoxedTypes || !isBoxedSlotType(type)) return;
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        Gp addr = cc.new_gp64();
        cc.lea(addr, Mem(s.boxedBase, static_cast<int32_t>(stackIdx * valueSize)));
        emitUnbox(s, addr, stackIdx, targetType);
    }

    void emitBoxOrCopy(JitEmissionState& s, Gp destAddr,
                       int stackOff, SlotType type)
    {
        if (s.usesBoxedTypes && isBoxedSlotType(type))
        {
            auto& cc = s.cc;
            constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
            Gp srcAddr = cc.new_gp64();
            cc.lea(srcAddr, Mem(s.boxedBase, static_cast<int32_t>(stackOff * valueSize)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            inv->set_arg(0, destAddr);
            inv->set_arg(1, srcAddr);
        }
        else
        {
            emitBox(s, destAddr, stackOff, type);
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
        emitBoxOrCopy(s, leftAddr, s.stackDepth - 1, lType);

        Gp rightAddr = cc.new_gp64();
        cc.lea(rightAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)
                              + static_cast<int32_t>(valueSize)));
        emitBoxOrCopy(s, rightAddr, s.stackDepth, rType);

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

        if (isBoxedSlotType(lType) || isBoxedSlotType(rType))
        {
            bool bothBoxed = isBoxedSlotType(lType) && isBoxedSlotType(rType);
            bool mixedWithPrimitive = !bothBoxed;

            if (mixedWithPrimitive)
            {
                SlotType target = !isBoxedSlotType(lType) ? lType : rType;
                if (target == SlotType::BOOL) target = SlotType::INT;
                emitEnsureUnboxed(s, s.stackDepth - 1, lType,
                    target == SlotType::FLOAT ? SlotType::FLOAT : SlotType::INT);
                emitEnsureUnboxed(s, s.stackDepth, rType,
                    target == SlotType::FLOAT ? SlotType::FLOAT : SlotType::INT);
                lType = target;
                rType = target;
            }
            else
            {
                if (kind == CmpOp::EQ || kind == CmpOp::NE)
                {
                    constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

                    Gp leftAddr = cc.new_gp64();
                    cc.lea(leftAddr, Mem(s.boxedBase,
                        static_cast<int32_t>((s.stackDepth - 1) * valueSize)));

                    Gp rightAddr = cc.new_gp64();
                    cc.lea(rightAddr, Mem(s.boxedBase,
                        static_cast<int32_t>(s.stackDepth * valueSize)));

                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_values_equal),
                              FuncSignature::build<int64_t, const value::Value*,
                                                   const value::Value*>());
                    inv->set_arg(0, leftAddr);
                    inv->set_arg(1, rightAddr);
                    inv->set_ret(0, result);

                    if (kind == CmpOp::NE)
                        cc.xor_(result, 1);

                    cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), result);
                    s.slotTypes.push_back(SlotType::BOOL);
                    return;
                }
                else
                {
                    // For ordering comparisons (LT, GT, LE, GE) on boxed values,
                    // unbox both sides to INT and fall through to the integer comparison path
                    emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT);
                    emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::INT);
                    lType = SlotType::INT;
                    rType = SlotType::INT;
                }
            }
        }

        if (lType == SlotType::FLOAT || rType == SlotType::FLOAT)
        {
            Vec right = cc.new_xmm();
            Vec left = cc.new_xmm();
            cc.movsd(right, Mem(s.stackBase, s.stackDepth * 8));
            cc.movsd(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            cc.ucomisd(left, right);
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

    static void emitUnboxParamBoxedMode(Compiler& cc, Gp localsBase,
                                         Gp argAddr, SlotType paramType,
                                         size_t localStride, size_t slot)
    {
        Gp destAddr = cc.new_gp64();
        cc.lea(destAddr, Mem(localsBase, static_cast<int32_t>(slot * localStride)));
        if (paramType == SlotType::FLOAT)
        {
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_float),
                      FuncSignature::build<double, const value::Value*>());
            inv->set_arg(0, argAddr);
            Vec val = cc.new_xmm();
            inv->set_ret(0, val);
            InvokeNode* boxInv;
            cc.invoke(Out(boxInv), reinterpret_cast<uint64_t>(jit_box_float),
                      FuncSignature::build<void, value::Value*, double>());
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

    static void emitUnboxParamRawMode(Compiler& cc, Gp localsBase,
                                       Gp argAddr, SlotType paramType, size_t slot)
    {
        if (paramType == SlotType::FLOAT)
        {
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_float),
                      FuncSignature::build<double, const value::Value*>());
            inv->set_arg(0, argAddr);
            Vec val = cc.new_xmm();
            inv->set_ret(0, val);
            cc.movsd(Mem(localsBase, static_cast<int32_t>(slot * 8)), val);
        }
        else
        {
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_int),
                      FuncSignature::build<int64_t, const value::Value*>());
            inv->set_arg(0, argAddr);
            Gp val = cc.new_gp64();
            inv->set_ret(0, val);
            cc.mov(Mem(localsBase, static_cast<int32_t>(slot * 8)), val);
        }
    }

    static void emitArgumentUnboxing(Compiler& cc, Gp ctxPtr, Gp localsBase,
                                      const bytecode::BytecodeProgram::FunctionMetadata& funcMeta,
                                      bool usesBoxedTypes, size_t localStride,
                                      std::unordered_map<size_t, SlotType>& localTypes)
    {
        constexpr size_t valueSize = sizeof(value::Value);
        Gp argsPtr = cc.new_gp64("argsPtr");
        cc.mov(argsPtr, Mem(ctxPtr, offsetof(JitContext, args)));

        for (size_t i = 0; i < funcMeta.parameterCount; ++i)
        {
            SlotType paramType = SlotType::INT;
            if (i < funcMeta.parameterTypes.size())
            {
                const std::string& t = funcMeta.parameterTypes[i];
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
                emitUnboxParamBoxedMode(cc, localsBase, argAddr, paramType, localStride, i);
            }
            else
            {
                emitUnboxParamRawMode(cc, localsBase, argAddr, paramType, i);
            }
        }
    }

    struct CompilationFrame
    {
        Gp localsBase;
        Gp stackBase;
        Gp boxedBase;
        Gp progPtr;
        bool usesBoxedTypes;
        size_t localCount;
        size_t localStride;
        std::unordered_map<size_t, SlotType> localTypes;
    };

    static CompilationFrame setupCompilationFrame(
        Compiler& cc, const bytecode::BytecodeProgram& program,
        const bytecode::BytecodeProgram::FunctionMetadata& funcMeta,
        size_t localCount)
    {
        constexpr size_t MAX_OP_STACK = 64;
        constexpr size_t valueSize = sizeof(value::Value);

        size_t scanEnd = funcMeta.startOffset + funcMeta.instructionCount;
        bool usesBoxedTypes = scanOpcodesForBoxedTypes(program, funcMeta.startOffset, scanEnd);
        if (!usesBoxedTypes)
        {
            for (const auto& t : funcMeta.parameterTypes)
            {
                if (t != "int" && t != "float" && t != "bool")
                {
                    usesBoxedTypes = true;
                    break;
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
            cc.mov(progPtr, reinterpret_cast<uint64_t>(&program));

        if (usesBoxedTypes)
            emitMemoryInit(cc, localsBase, localCount, boxedBase, MAX_OP_STACK);

        std::unordered_map<size_t, SlotType> localTypes;
        return {localsBase, stackBase, boxedBase, progPtr,
                usesBoxedTypes, localCount, localStride, std::move(localTypes)};
    }

    static void emitCodegenLoop(JitEmissionState& s,
                                size_t startOffset, size_t instrCount,
                                const bytecode::BytecodeProgram& program)
    {
        for (size_t ip = startOffset; ip < startOffset + instrCount && !s.compileFailed; ++ip)
        {
            auto labelIt = s.labels.find(ip);
            if (labelIt != s.labels.end())
            {
                s.cc.bind(labelIt->second);
                if (s.backEdgeTargets.find(ip) == s.backEdgeTargets.end())
                    s.arrayInfoCache.clear();
            }

            const auto& instr = program.getInstruction(ip);
            s.currentIP = ip;
            bool handled = false;
            if (emitCoreOps(s, instr)) { handled = true; }
            else if (emitArithmeticOps(s, instr)) { handled = true; }
            else if (emitControlFlowOps(s, instr, nullptr)) { handled = true; }
            else { emitObjectOps(s, instr); handled = true; }

            if (s.compileFailed)
                break;
        }
    }

    static bool emitFunctionBody(Compiler& cc, Gp ctxPtr,
                                   const bytecode::BytecodeProgram& program,
                                   const bytecode::BytecodeProgram::FunctionMetadata& funcMeta,
                                   CompilationFrame& frame,
                                   ic::TypeFeedbackCollector* typeFeedback)
    {
        emitArgumentUnboxing(cc, ctxPtr, frame.localsBase, funcMeta,
                             frame.usesBoxedTypes, frame.localStride, frame.localTypes);

        if (!frame.usesBoxedTypes)
        {
            for (size_t i = funcMeta.parameterCount; i < funcMeta.localCount; ++i)
                cc.mov(qword_ptr(frame.localsBase, static_cast<int32_t>(i * 8)), 0);
        }

        size_t startOffset = funcMeta.startOffset;
        size_t instrCount = funcMeta.instructionCount;
        auto labels = createJumpLabels(cc, program, startOffset, startOffset + instrCount);
        auto backEdges = collectBackEdgeTargets(program, startOffset, startOffset + instrCount);

        JitEmissionState s{cc, ctxPtr, frame.localsBase, frame.stackBase,
                           frame.boxedBase, frame.progPtr,
                           frame.usesBoxedTypes, frame.localCount, frame.localStride,
                           0, {}, frame.localTypes, false, 0, labels, program,
                           typeFeedback, {}, backEdges};

        emitCodegenLoop(s, startOffset, instrCount, program);

        if (s.compileFailed)
            return false;

        emitCleanup(s);
        cc.mov(byte_ptr(ctxPtr, offsetof(JitContext, hasReturnValue)), 0);
        return true;
    }

    bool JitCompiler::compile(const std::string& functionName,
                               const bytecode::BytecodeProgram& program,
                               JitCodeCache& codeCache,
                               ic::TypeFeedbackCollector* typeFeedback)
    {
        if (codeCache.contains(functionName))
            return true;

        const auto* funcMeta = program.getFunction(functionName);
        if (!funcMeta)
        {
            bailoutCount++;
            return false;
        }
        if (!canCompile(*funcMeta, program))
        {
            bailoutCount++;
            return false;
        }

        size_t localCount = funcMeta->localCount;
        if (localCount == 0) localCount = 1;
        constexpr size_t MAX_LOCAL_COUNT = 1024;
        if (localCount > MAX_LOCAL_COUNT)
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

        auto frame = setupCompilationFrame(cc, program, *funcMeta, localCount);

        if (!emitFunctionBody(cc, ctxPtr, program, *funcMeta, frame, typeFeedback))
        {
            bailoutCount++;
            return false;
        }

        cc.end_func();
        return finalizeAndStore(cc, code, codeCache, functionName,
                               compileCount, bailoutCount);
    }
}
