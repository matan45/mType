#include "JitEmissionState.hpp"
#include "JitCompiler_Objects.hpp"
#include "JitCompiler_ControlFlow.hpp"
#include "JitCompiler.hpp"
#include "JitCodeCache.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include "../optimization/InlineAnalysis.hpp"
#include "../optimization/analysis/DataFlowAnalyzer.hpp"
#include "../../environment/registry/ClassDefinition.hpp"
#include <asmjit/x86.h>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    // Walk the callee's bytecode and return the peak operand-stack depth it
    // reaches starting from depth 0. Inline guards in the method/function
    // inliners use this to reject candidates whose caller_depth + callee_peak
    // would exceed MAX_OP_STACK and overrun cc.new_stack — that overrun trips
    // the /GS-cookie fastfail. For known opcodes we use
    // DataFlowAnalyzer::calculateStackEffect; for opcodes it doesn't classify
    // (CALL_METHOD, CALL_FAST, NEW_INSTANCE, lambda invokes, iterator ops) we
    // default to a conservative net +1. Throws inside the body return
    // MAX_OP_STACK+1 so every caller rejects the candidate cleanly.
    size_t computeCalleePeakOperandStack(
        const bytecode::BytecodeProgram& program,
        const bytecode::BytecodeProgram::FunctionMetadata& callee)
    {
        try
        {
            using DFA = optimization::analysis::DataFlowAnalyzer;
            int depth = 0;
            int peak  = 0;
            const size_t end = callee.startOffset + callee.instructionCount;
            for (size_t ip = callee.startOffset; ip < end; ++ip)
            {
                const auto& instr = program.getInstruction(ip);
                DFA::StackEffect e = DFA::calculateStackEffect(instr.opcode);
                int net = e.netEffect;
                if (e.consumed == 0 && e.produced == 0
                    && instr.opcode != bytecode::OpCode::NOP
                    && instr.opcode != bytecode::OpCode::JUMP
                    && instr.opcode != bytecode::OpCode::JUMP_BACK
                    && instr.opcode != bytecode::OpCode::LINE
                    && instr.opcode != bytecode::OpCode::SOURCE_FILE
                    && instr.opcode != bytecode::OpCode::LOOP_START
                    && instr.opcode != bytecode::OpCode::LOOP_END
                    && instr.opcode != bytecode::OpCode::SWAP
                    && instr.opcode != bytecode::OpCode::RETURN)
                {
                    net = 1;
                }
                depth += net;
                if (depth < 0) depth = 0;
                if (depth > peak) peak = depth;
            }
            return static_cast<size_t>(peak);
        }
        catch (...)
        {
            return JitEmissionState::MAX_OP_STACK + 1;
        }
    }

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
        // popType pops from top-to-bottom, so match with top-to-bottom indices
        for (size_t i = 0; i < argCount; ++i)
        {
            SlotType at = popType(s);
            if (s.usesBoxedTypes && isBoxedSlotType(at))
            {
                // i=0 is the topmost arg (at stackDepth-1), i=1 is next, etc.
                int idx = s.stackDepth - 1 - static_cast<int>(i);
                emitValueDestroy(s, idx);
            }
        }
        s.stackDepth -= static_cast<int>(argCount);
    }

    InlineEmitStateSnapshot snapshotEmitStateForInline(const JitEmissionState& s)
    {
        return { s.stackDepth, s.slotTypes, s.localTypes, s.arrayInfoCache, s.currentIP };
    }

    void restoreEmitStateForInline(JitEmissionState& s,
                                    const InlineEmitStateSnapshot& snap)
    {
        s.stackDepth     = snap.stackDepth;
        s.slotTypes      = snap.slotTypes;
        s.localTypes     = snap.localTypes;
        s.arrayInfoCache = snap.arrayInfoCache;
        s.currentIP      = snap.currentIP;
    }

    void normalizeInlineReturnJoinState(JitEmissionState& s,
                                         int resultStackIdx,
                                         size_t callSiteIP)
    {
        s.stackDepth = resultStackIdx + 1;
        s.slotTypes.resize(static_cast<size_t>(resultStackIdx));
        s.slotTypes.push_back(SlotType::BOXED);
        s.currentIP = callSiteIP;
        s.arrayInfoCache.clear();
    }

    // Stable storage for inlined-callee class names so the const char* baked
    // into JIT-emitted cc.invoke immediates remains valid for the lifetime
    // of the JIT'd code. std::deque guarantees stable element addresses
    // across appends (unlike std::vector), and std::string's heap buffer is
    // stable as long as the std::string itself isn't moved — which it isn't.
    static const char* internInlinedClassName(std::string&& name)
    {
        static std::deque<std::string> storage;
        storage.push_back(std::move(name));
        return storage.back().c_str();
    }

    // Derive owner-class name from the callee's qualified name. For class
    // methods registered by MethodCompilerHelper, `metadata.name` is the
    // qualified name "ClassName::method/sig" while `mangledName` is empty
    // (only FunctionCompiler sets mangledName, for free functions). For
    // instance methods we prefer `name`; if it lacks "::" we fall back to
    // mangledName, then to empty (free function — the push pushes "" which
    // the validator falls back from).
    static const char* deriveOwnerClassNameCStr(
        const bytecode::BytecodeProgram::FunctionMetadata& callee)
    {
        auto extractClass = [](const std::string& s) -> std::string {
            size_t pos = s.find("::");
            return pos == std::string::npos ? std::string() : s.substr(0, pos);
        };
        std::string fromName = extractClass(callee.name);
        if (!fromName.empty()) return internInlinedClassName(std::move(fromName));
        std::string fromMangled = extractClass(callee.mangledName);
        return internInlinedClassName(std::move(fromMangled));
    }

    void emitInlineCalleeBody(
        JitEmissionState& s,
        const bytecode::BytecodeProgram::FunctionMetadata& callee,
        int stackBaselineIdx,
        const ExitHandler& onExit)
    {
        (void)stackBaselineIdx;
        auto& cc = s.cc;

        // Push the callee's owner class onto ctx->inlinedCallingClassNames so
        // private/protected field-access checks inside the inlined body are
        // validated against the callee's owner class, not the outer function's.
        // Without this an OSR-inlined `ListIterator::hasNext` body running
        // inside a FilteringIterator OSR loop throws AccessViolationException
        // on its own `this.currentIndex` read.
        //
        // Emitted as a few inline instructions (not cc.invoke) so per-inlined-
        // call overhead is ~3 mov + 1 inc. Pop is emitted at fall-through and
        // inside each RETURN_VALUE's onExit handler before the jmp.
        const char* ownerClassCstr = deriveOwnerClassNameCStr(callee);
        {
            Gp nameReg = cc.new_gp64();
            cc.mov(nameReg, reinterpret_cast<uint64_t>(ownerClassCstr));
            InvokeNode* push = nullptr;
            cc.invoke(Out(push),
                      reinterpret_cast<uint64_t>(jit_push_inlined_class),
                      FuncSignature::build<void, JitContext*, const char*>());
            push->set_arg(0, s.ctxPtr);
            push->set_arg(1, nameReg);
        }

        for (size_t ip = callee.startOffset;
             ip < callee.startOffset + callee.instructionCount && !s.compileFailed;
             ++ip)
        {
            auto& topFrame = s.inlineStack.back();
            auto lit = topFrame.localJumpLabels.find(ip);
            if (lit != topFrame.localJumpLabels.end())
            {
                // Match emitCodegenLoop / emitOSRCodegenLoop's behavior: flush
                // hints, bind, invalidate hints, clear arrayInfoCache when not
                // a back-edge target. Do NOT touch stackDepth/slotTypes — the
                // bytecode-compiler emits jumps that converge to a consistent
                // depth on both paths. Resetting them broke short-circuit
                // operators (JUMP_IF_TRUE_OR_POP / JUMP_IF_FALSE_OR_POP target
                // labels land mid-expression with one value still on stack;
                // the reset drove stackDepth negative).
                flushAllHints(s);
                cc.bind(lit->second);
                invalidateAllHints(s);
                s.arrayInfoCache.clear();
            }

            const auto& cinstr = s.program.getInstruction(ip);
            s.currentIP = ip;
            if (cinstr.opcode == OpCode::PROFILE_ENTER)
                continue;  // no-op inside an inline frame

            bool handled = false;
            if (emitCoreOps(s, cinstr)) handled = true;
            else if (emitArithmeticOps(s, cinstr)) handled = true;
            else if (emitControlFlowOps(s, cinstr, onExit)) handled = true;
            else { emitObjectOps(s, cinstr); handled = true; }
            (void)handled;
        }

        // Fall-through pop (paired with the push above). Reachable only when
        // the body has no terminating RETURN_VALUE — a degenerate case whose
        // RETURN_VALUE-emitted onExit handler also pops. Both pop sites are
        // emitted at compile time; only one runs per execution because
        // RETURN_VALUE jumps to endLabel before this fall-through is reached.
        {
            InvokeNode* pop = nullptr;
            cc.invoke(Out(pop),
                      reinterpret_cast<uint64_t>(jit_pop_inlined_class),
                      FuncSignature::build<void, JitContext*>());
            pop->set_arg(0, s.ctxPtr);
        }
    }

    static bool isInlineStaticPrimitiveType(const std::string& type)
    {
        return type == "int" || type == "float" || type == "bool";
    }

    // Static CALL_STATIC sites are monomorphic in the bytecode operand. For
    // small non-generic primitive callees, paste the callee body into the
    // caller instead of routing every iteration through jit_call_function_ic's
    // mini-interpreter frame.
    bool tryEmitInlinedStaticCall(
        JitEmissionState& s,
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto recordDecision = [&s](optimization::InlineDecision decision) {
            if (s.inlineDecisions)
                s.inlineDecisions->bumpFunction(decision);
        };

        const uint32_t nameIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
        const size_t argCount = instr.inlineOperands[1];
        if (nameIndex >= s.program.getConstantPool().strings.size())
        {
            recordDecision(optimization::InlineDecision::UNKNOWN_SHAPE);
            return false;
        }
        if (!s.usesBoxedTypes)
        {
            recordDecision(optimization::InlineDecision::UNKNOWN_SHAPE);
            return false;
        }

        const std::string& funcName = s.program.getConstantPool().getString(nameIndex);
        const auto* callee = s.program.getFunction(funcName);
        if (!callee)
        {
            recordDecision(optimization::InlineDecision::CALLEE_NOT_FOUND);
            return false;
        }
        if (callee->parameterCount != argCount)
        {
            recordDecision(optimization::InlineDecision::UNKNOWN_SHAPE);
            return false;
        }

        // CALL_STATIC-specific gates: no generic instantiations, primitive-only
        // signatures. These keep the static-call slow path simple and predate
        // the shared-inliner generalisation; if either proves over-restrictive
        // for static sites, lift them into the shared helper.
        if (!callee->genericTypeParameters.empty())
        {
            recordDecision(optimization::InlineDecision::UNKNOWN_SHAPE);
            return false;
        }
        for (const auto& paramType : callee->parameterTypes)
        {
            if (!isInlineStaticPrimitiveType(paramType))
            {
                recordDecision(optimization::InlineDecision::UNKNOWN_SHAPE);
                return false;
            }
        }
        if (!isInlineStaticPrimitiveType(callee->returnType))
        {
            recordDecision(optimization::InlineDecision::UNKNOWN_SHAPE);
            return false;
        }

        // Register the reverse edge so a redefinition (rare for CALL_STATIC,
        // but defensive) evicts every caller that pasted this callee's body.
        const auto handle = s.program.internFrameName(funcName);
        return tryEmitInlinedFunctionCall(s, callee, argCount, handle);
    }

    // Shared inline emitter for plain (non-method) function calls. Used by
    // tryEmitInlinedStaticCall (CALL_STATIC) and by emitCallOp / emitCallFastOp
    // (plain CALL / CALL_FAST) in JitCompiler_ControlFlow.cpp.
    //
    // The caller is expected to have resolved `callee` and to pass a fresh
    // `argCount` (no implicit `this`). Boxed mode is required: the inlined
    // body emits in boxed-mode conventions and the locals-copy assumes that
    // emission style.
    //
    // No runtime identity guard. Identity safety is provided by eager
    // invalidation: on success the helper records a caller→callee edge in
    // JitCodeCache via registerInlineEdge, and a subsequent
    // invalidatedInlineCallersOf(callee) returns this caller's name so its
    // JIT buffer can be evicted. Pass INVALID_FN_HANDLE to skip bookkeeping
    // (best-effort safety — caller accepts residual stale-code risk).
    bool tryEmitInlinedFunctionCall(
        JitEmissionState& s,
        const bytecode::BytecodeProgram::FunctionMetadata* callee,
        size_t argCount,
        bytecode::FunctionNameHandle calleeHandle)
    {
        auto recordDecision = [&s](optimization::InlineDecision decision) {
            if (s.inlineDecisions)
                s.inlineDecisions->bumpFunction(decision);
        };

        if (!s.usesBoxedTypes)
        {
            recordDecision(optimization::InlineDecision::UNKNOWN_SHAPE);
            return false;
        }
        if (!callee)
        {
            recordDecision(optimization::InlineDecision::CALLEE_NOT_FOUND);
            return false;
        }
        if (callee->parameterCount != argCount)
        {
            recordDecision(optimization::InlineDecision::UNKNOWN_SHAPE);
            return false;
        }

        auto decision = optimization::checkFunctionInlineEligibility(
            s.program, *callee, s.currentCompilingFn, s.inlineStack.size());
        recordDecision(decision);
        if (decision != optimization::InlineDecision::INLINE)
            return false;

        const size_t localsBaseSlot = s.inlineStack.empty()
            ? s.localCount
            : s.inlineStack.back().localsBaseSlot
              + s.inlineStack.back().calleeMeta->localCount;
        if (localsBaseSlot + callee->localCount
            > s.localCount + JitEmissionState::INLINE_LOCALS_SLACK)
        {
            recordDecision(optimization::InlineDecision::CALLEE_TOO_BIG);
            return false;
        }

        const size_t calleePeak = computeCalleePeakOperandStack(s.program, *callee);
        if (static_cast<size_t>(s.stackDepth) + calleePeak
            > JitEmissionState::MAX_OP_STACK)
        {
            recordDecision(optimization::InlineDecision::CALLEE_TOO_BIG);
            return false;
        }

        auto& cc = s.cc;
        Label endLabel = cc.new_label();
        auto localJumpLabels = createJumpLabels(
            cc, s.program, callee->startOffset,
            callee->startOffset + callee->instructionCount);

        const InlineEmitStateSnapshot snap = snapshotEmitStateForInline(s);
        const int firstArgStackIdx = snap.stackDepth - static_cast<int>(argCount);
        if (firstArgStackIdx < 0)
        {
            recordDecision(optimization::InlineDecision::UNKNOWN_SHAPE);
            return false;
        }

        emitInlineLocalCopy(s, firstArgStackIdx, localsBaseSlot, *callee);

        InlineFrame frame;
        frame.calleeMeta = callee;
        frame.localsBaseSlot = localsBaseSlot;
        frame.inlineEndLabel = endLabel;
        frame.returnResultStackIdx = firstArgStackIdx;
        frame.localJumpLabels = std::move(localJumpLabels);

        const size_t prevInlineBase = s.inlineLocalsBase;
        s.inlineStack.push_back(frame);
        s.inlineLocalsBase = localsBaseSlot;

        s.slotTypes.resize(s.slotTypes.size() - argCount);
        s.stackDepth = firstArgStackIdx;
        s.arrayInfoCache.clear();

        ExitHandler onExit = [endLabel, firstArgStackIdx, localsBaseSlot, callee]
            (JitEmissionState& es, size_t /*target*/) {
            emitInlineReturnMaterialize(es, firstArgStackIdx, es.lastReturnSlotType);
            emitInlineLocalDestroy(es, localsBaseSlot, *callee);
            {
                InvokeNode* pop = nullptr;
                es.cc.invoke(Out(pop),
                          reinterpret_cast<uint64_t>(jit_pop_inlined_class),
                          FuncSignature::build<void, JitContext*>());
                pop->set_arg(0, es.ctxPtr);
            }
            es.cc.jmp(endLabel);
        };

        emitInlineCalleeBody(s, *callee, firstArgStackIdx, onExit);

        s.inlineStack.pop_back();
        s.inlineLocalsBase = prevInlineBase;
        s.currentIP = snap.currentIP;

        if (s.compileFailed)
            return true;

        cc.jmp(endLabel);
        cc.bind(endLabel);

        s.stackDepth = firstArgStackIdx + 1;
        s.slotTypes.resize(static_cast<size_t>(firstArgStackIdx));
        s.slotTypes.push_back(SlotType::BOXED);
        s.arrayInfoCache.clear();

        // Register the reverse edge so a later redefinition of `callee` will
        // evict this caller's JIT buffer. Only meaningful when we have a
        // static caller name to evict (currentCompilingFn empty in OSR —
        // record nothing, leaving best-effort safety).
        if (calleeHandle != bytecode::INVALID_FN_HANDLE
            && !s.currentCompilingFn.empty()
            && s.codeCache)
        {
            s.codeCache->registerInlineEdge(calleeHandle, s.currentCompilingFn);
        }
        return true;
    }

}
