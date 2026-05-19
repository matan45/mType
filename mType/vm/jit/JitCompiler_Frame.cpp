#include "JitCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    namespace
    {
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

        // Replaces the previous 7 by-pointer counter params on emitFunctionBody.
        // Pointers are baked as immediates at emit time, so the addresses must
        // live as long as any compiled function — JitCompiler is owned by the
        // VirtualMachine, so this is satisfied.
        struct JitCompileMetrics
        {
            uint64_t* inlineFieldICHits;
            uint64_t* inlineFieldICMisses;
            uint64_t* inlineFieldSetICHits;
            uint64_t* inlineFieldSetICMisses;
            InlineDecisionCounters* inlineDecisions;
            uint64_t* tailCallsOptimized;
            uint64_t* selfDirectCalls;
        };

        void emitUnboxParamBoxedMode(Compiler& cc, Gp localsBase,
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

        void emitUnboxParamRawMode(Compiler& cc, Gp localsBase,
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

        void emitArgumentUnboxing(Compiler& cc, Gp ctxPtr, Gp localsBase,
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

        CompilationFrame setupCompilationFrame(
            Compiler& cc, const bytecode::BytecodeProgram& program,
            const bytecode::BytecodeProgram::FunctionMetadata& funcMeta,
            size_t localCount)
        {
            // MYT-251: use the single source of truth in JitEmissionState so
            // all three setup paths (here, OSR, and the emitters' bounds
            // checks) share one value.
            constexpr size_t MAX_OP_STACK = JitEmissionState::MAX_OP_STACK;
            constexpr size_t valueSize = sizeof(value::Value);

            size_t scanEnd = funcMeta.startOffset + funcMeta.instructionCount;
            // MYT-316: pass the enclosing function's identity so plain CALL /
            // CALL_FAST sites targeting the same function (self-recursion)
            // don't flip the function into boxed mode — the inliner would
            // reject those anyway, and TCO / self-direct-call need unboxed
            // mode to fire.
            const std::string& selfFnName = funcMeta.mangledName.empty()
                ? funcMeta.name : funcMeta.mangledName;
            bool usesBoxedTypes = scanOpcodesForBoxedTypes(program, funcMeta.startOffset, scanEnd,
                                                            selfFnName);
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

            // MYT-163: reserve INLINE_LOCALS_SLACK extra slots so
            // tryEmitInlinedMethodCall can stage a callee's locals window
            // without re-allocating the frame.
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

            // MYT-163: zero-init all reserved locals (including the inline
            // slack) so an inlined callee's locals area starts in a clean
            // state — no destructor calls on uninitialised shared_ptr bytes.
            if (usesBoxedTypes)
                emitMemoryInit(cc, localsBase, reservedLocals, boxedBase, MAX_OP_STACK);

            std::unordered_map<size_t, SlotType> localTypes;
            return {localsBase, stackBase, boxedBase, progPtr,
                    usesBoxedTypes, localCount, localStride, std::move(localTypes)};
        }

        void emitCodegenLoop(JitEmissionState& s,
                              size_t startOffset, size_t instrCount,
                              const bytecode::BytecodeProgram& program)
        {
            for (size_t ip = startOffset; ip < startOffset + instrCount && !s.compileFailed; ++ip)
            {
                auto labelIt = s.labels.find(ip);
                if (labelIt != s.labels.end())
                {
                    // MYT-211: ensure memory is coherent for the fall-through
                    // path before binding (any dirty hint from the previous
                    // emit must hit memory now). After bind, drop hints — a
                    // forward jump arriving here may have its own register
                    // state.
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

        bool emitFunctionBody(Compiler& cc, Gp ctxPtr,
                               const bytecode::BytecodeProgram& program,
                               const bytecode::BytecodeProgram::FunctionMetadata& funcMeta,
                               CompilationFrame& frame,
                               ic::TypeFeedbackCollector* typeFeedback,
                               JitCodeCache* codeCache,  // MYT-315
                               asmjit::Label selfFuncCallLabel,
                               const JitCompileMetrics& metrics)
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
            s.inlineFieldICHits = metrics.inlineFieldICHits;
            s.inlineFieldICMisses = metrics.inlineFieldICMisses;
            s.inlineFieldSetICHits = metrics.inlineFieldSetICHits;
            s.inlineFieldSetICMisses = metrics.inlineFieldSetICMisses;
            s.inlineDecisions = metrics.inlineDecisions;
            s.tailCallsOptimized = metrics.tailCallsOptimized;
            s.selfDirectCalls = metrics.selfDirectCalls;
            s.codeCache = codeCache;  // MYT-315: fresh JIT lookup at compile time

            // MYT-207: expose the FuncNode's entry label so non-tail self-
            // recursive CALL sites can `cc.invoke(label, sig)` directly,
            // skipping the jit_call_function dispatch overhead.
            s.selfFuncCallLabel = selfFuncCallLabel;
            s.selfDirectCallEnabled = !frame.usesBoxedTypes;

            // Phase 1 (self-recursive TCO): bind the function-entry label
            // right after argument unboxing and local init, so a tail self-
            // call can jmp back here after overwriting locals with new arg
            // values without re-running the ctx->args unboxing prologue.
            // Gated on the same precondition TCO uses (!usesBoxedTypes) —
            // binding a bound-but-unreachable label in a boxed frame
            // corrupted asmjit codegen for functions containing InvokeNode
            // arg unboxing (e.g. int[] params).
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

            // MYT-268: bind the per-function deopt-exit label after the
            // normal epilogue. emitAwaitOp emits cc.jnz to this label after
            // stashing OSRDeoptException via jit_await; we want it reached
            // only via that jump. Emit an explicit cc.ret() so the post-loop
            // fall-through cleanup above can't fall into the deopt-exit
            // cleanup (which would double-destroy boxed locals via
            // emitCleanup's jit_value_destroy invokes). The hasAwaitDeoptExit
            // guard avoids binding an unreferenced label in functions with
            // no AWAIT.
            if (s.hasAwaitDeoptExit)
            {
                cc.ret();
                cc.bind(s.functionDeoptExitLabel);
                emitCleanup(s);
                cc.mov(byte_ptr(ctxPtr, offsetof(JitContext, hasReturnValue)), 0);
                cc.ret();
            }
            return true;
        }
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
        OpCode offendingOpcode = OpCode::OPCODE_SENTINEL_;
        if (!canCompile(*funcMeta, program, &offendingOpcode))
        {
            bailoutCount++;
            if (offendingOpcode != OpCode::OPCODE_SENTINEL_)
                functionBailoutOpcodes[static_cast<uint8_t>(offendingOpcode)]++;
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

        JitCompileMetrics metrics{
            &inlineFieldICHits, &inlineFieldICMisses,
            &inlineFieldSetICHits, &inlineFieldSetICMisses,
            &inlineDecisions,
            &tailCallsOptimized, &selfDirectCalls,
        };

        if (!emitFunctionBody(cc, ctxPtr, program, *funcMeta, frame, typeFeedback,
                               &codeCache, func->label(), metrics))
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
