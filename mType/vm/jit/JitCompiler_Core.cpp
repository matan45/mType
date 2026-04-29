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
                                         const bytecode::BytecodeProgram& program,
                                         uint8_t* outOpcode) const
    {
        const auto& supported = getSupportedOpcodes();
        for (size_t ip = loopStartOffset; ip <= loopEndOffset; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            uint8_t op = static_cast<uint8_t>(instr.opcode);
            if (supported.find(op) == supported.end())
            {
                if (outOpcode) *outOpcode = op;
                return false;
            }
        }
        return true;
    }

    void emitBox(JitEmissionState& s, Gp destAddr, int stackOff, SlotType type)
    {
        // MYT-211: emitBox reads stackBase memory and emits a helper invoke;
        // flush so the read picks up any register-cached value at stackOff.
        flushAllHints(s);
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
        // MYT-211: emitUnbox crosses a cc.invoke boundary; flush any pending
        // dirty cache slots so the helper sees coherent memory. After the
        // unbox, record the result virtreg as a hint so the next consumer
        // skips re-reading stackBase[stackOff*8].
        flushAllHints(s);
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
            recordXmmHint(s, stackOff, val);
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
            recordGpHint(s, stackOff, val);
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
        // MYT-211: any pending register-cached slot must hit memory before
        // we ret — JIT-frame teardown happens here, and the inlined locals
        // cleanup helper (when usesBoxedTypes) reads the locals memory range.
        // Cheap no-op in the !boxed path when the cache is already empty.
        flushAllHints(s);
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
        // MYT-163: clean the inline slack too. Unwritten slack slots hold
        // monostate (zero-inited by setupCompilationFrame), so destroy is a
        // cheap no-op there — but any slot written by an inlined callee must
        // be destroyed to avoid shared_ptr leaks.
        cc.mov(lCount, static_cast<int64_t>(s.localCount + JitEmissionState::INLINE_LOCALS_SLACK));
        InvokeNode* cleanLocals;
        cc.invoke(Out(cleanLocals), reinterpret_cast<uint64_t>(jit_locals_cleanup),
                  FuncSignature::build<void, value::Value*, size_t>());
        cleanLocals->set_arg(0, lBase);
        cleanLocals->set_arg(1, lCount);
    }

    void emitGenericBinop(JitEmissionState& s, uint64_t helperFn,
                          SlotType lType, SlotType rType)
    {
        // MYT-211: this helper boxes operands via emitBoxOrCopy → cc.invoke,
        // and ends with another cc.invoke to the helper itself. Flush so the
        // box reads see the latest cached values.
        flushAllHints(s);
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

    // Returns true if comparison was fully handled (both-boxed EQ/NE).
    // Returns false if types were unboxed and caller should fall through to primitive path.
    bool emitCmpBoxed(JitEmissionState& s, CmpOp kind, Gp result,
                      SlotType& lType, SlotType& rType)
    {
        // MYT-211: boxed-cmp path uses cc.invoke and reads boxed memory; flush
        // before the box reads. No-op when cache is empty.
        flushAllHints(s);
        auto& cc = s.cc;
        bool bothBoxed = isBoxedSlotType(lType) && isBoxedSlotType(rType);

        if (!bothBoxed)
        {
            SlotType target = !isBoxedSlotType(lType) ? lType : rType;
            if (target == SlotType::BOOL) target = SlotType::INT;
            emitEnsureUnboxed(s, s.stackDepth - 1, lType,
                target == SlotType::FLOAT ? SlotType::FLOAT : SlotType::INT);
            emitEnsureUnboxed(s, s.stackDepth, rType,
                target == SlotType::FLOAT ? SlotType::FLOAT : SlotType::INT);
            lType = target;
            rType = target;
            return false;
        }

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

            return true;
        }

        emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT);
        emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::INT);
        lType = SlotType::INT;
        rType = SlotType::INT;
        return false;
    }

    void emitCmpPrimitive(JitEmissionState& s, CmpOp kind, Gp result,
                          SlotType lType, SlotType rType)
    {
        // MYT-211: consume operands via hint helpers; cache disabled means
        // they read memory, but the indirection is safe.
        auto& cc = s.cc;

        if (lType == SlotType::FLOAT || rType == SlotType::FLOAT)
        {
            Vec right = consumeXmmHint(s, s.stackDepth);
            Vec left = consumeXmmHint(s, s.stackDepth - 1);
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
            Gp right = consumeGpHint(s, s.stackDepth);
            Gp left = consumeGpHint(s, s.stackDepth - 1);
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
            if (emitCmpBoxed(s, kind, result, lType, rType))
            {
                cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), result);
                s.slotTypes.push_back(SlotType::BOOL);
                return;
            }
        }

        emitCmpPrimitive(s, kind, result, lType, rType);
        // MYT-211: write the boolean result via publishGpHint (always writes
        // stackBase). Same final memory state as the original cc.mov.
        s.slotTypes.push_back(SlotType::BOOL);
        publishGpHint(s, s.stackDepth - 1, result);
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

        // MYT-163: reserve INLINE_LOCALS_SLACK extra slots so tryEmitInlinedMethodCall
        // can stage a callee's locals window without re-allocating the frame.
        // Conservative for F-a — F-b will replace with a per-function pre-scan.
        const size_t reservedLocals = localCount + JitEmissionState::INLINE_LOCALS_SLACK;

        Mem localsArea = cc.new_stack(static_cast<uint32_t>(reservedLocals * localStride), 8);
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

        // MYT-163: zero-init all reserved locals (including the inline slack)
        // so an inlined callee's locals area starts in a clean state — no
        // destructor calls on uninitialised shared_ptr bytes.
        if (usesBoxedTypes)
            emitMemoryInit(cc, localsBase, reservedLocals, boxedBase, MAX_OP_STACK);

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
                // MYT-211: ensure memory is coherent for the fall-through path
                // before binding (any dirty hint from the previous emit must
                // hit memory now). After bind, drop hints — a forward jump
                // arriving here may have its own register state.
                flushAllHints(s);
                s.cc.bind(labelIt->second);
                invalidateAllHints(s);
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

            if (s.compileFailed) break;
        }
    }

    static bool emitFunctionBody(Compiler& cc, Gp ctxPtr,
                                   const bytecode::BytecodeProgram& program,
                                   const bytecode::BytecodeProgram::FunctionMetadata& funcMeta,
                                   CompilationFrame& frame,
                                   ic::TypeFeedbackCollector* typeFeedback,
                                   uint64_t* inlineFieldICHits,
                                   uint64_t* inlineFieldICMisses,
                                   uint64_t* inlineFieldSetICHits,
                                   uint64_t* inlineFieldSetICMisses,
                                   InlineDecisionCounters* inlineDecisions,
                                   asmjit::Label selfFuncCallLabel,
                                   uint64_t* tailCallsOptimized,
                                   uint64_t* selfDirectCalls)
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
                           0, {}, /*slotHints=*/{}, /*invariantIntLocals=*/{},
                           /*invariantFloatLocals=*/{}, frame.localTypes, false,
                           OSRBailoutReason::NONE, 0,
                           0, labels, program,
                           typeFeedback, {}, backEdges};
        // MYT-163: record the function being compiled so the inlining
        // eligibility check can reject self-recursive inlining.
        s.currentCompilingFn = funcMeta.mangledName.empty()
            ? funcMeta.name : funcMeta.mangledName;
        s.inlineFieldICHits = inlineFieldICHits;
        s.inlineFieldICMisses = inlineFieldICMisses;
        s.inlineFieldSetICHits = inlineFieldSetICHits;
        s.inlineFieldSetICMisses = inlineFieldSetICMisses;
        s.inlineDecisions = inlineDecisions;
        s.tailCallsOptimized = tailCallsOptimized;
        s.selfDirectCalls = selfDirectCalls;

        // MYT-207: expose the FuncNode's entry label so non-tail self-recursive
        // CALL sites can `cc.invoke(label, sig)` directly, skipping the
        // jit_call_function dispatch overhead.
        s.selfFuncCallLabel = selfFuncCallLabel;
        s.selfDirectCallEnabled = !frame.usesBoxedTypes;

        // Phase 1 (self-recursive TCO): bind the function-entry label right
        // after argument unboxing and local init, so a tail self-call can
        // jmp back here after overwriting locals with new arg values without
        // re-running the ctx->args unboxing prologue. Gated on the same
        // precondition TCO uses (!usesBoxedTypes) — binding a bound-but-
        // unreachable label in a boxed frame corrupted asmjit codegen for
        // functions containing InvokeNode arg unboxing (e.g. int[] params).
        if (!frame.usesBoxedTypes)
        {
            s.functionEntryLabel = cc.new_label();
            s.tailCallCounter = cc.new_gp64("tailCallCounter");
            cc.xor_(s.tailCallCounter, s.tailCallCounter);  // MYT-226
            cc.bind(s.functionEntryLabel);
        }
        else
        {
            s.selfTailCallEnabled = false;
        }

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

        if (!emitFunctionBody(cc, ctxPtr, program, *funcMeta, frame, typeFeedback,
                               &inlineFieldICHits, &inlineFieldICMisses,
                               &inlineFieldSetICHits, &inlineFieldSetICMisses,
                               &inlineDecisions,
                               func->label(),
                               &tailCallsOptimized,
                               &selfDirectCalls))
        {
            bailoutCount++;
            return false;
        }

        cc.end_func();
        if (!finalizeAndStore(cc, code, codeCache, functionName,
                              compileCount, bailoutCount))
            return false;

        // Phase 2: populate the index-keyed fast-path slot so
        // jit_call_function_fast can skip the name-hashmap lookup. Using
        // `functionName` as-is — it's the same key the cache already stores
        // under (matches VirtualMachine::executeCallFastWithJit's lookup via
        // funcMeta->mangledName).
        JitFunction fn = codeCache.lookup(functionName);
        size_t funcIndex = program.getFunctionIndex(functionName);
        if (fn && funcIndex != SIZE_MAX)
        {
            auto frameName = program.internFrameName(functionName);
            codeCache.storeByIndex(funcIndex, fn, frameName);
        }
        return true;
    }
}
