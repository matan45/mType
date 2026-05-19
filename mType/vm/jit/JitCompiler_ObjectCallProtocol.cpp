#include "JitEmissionState.hpp"
#include "JitCompiler_Objects.hpp"
#include "JitHelpers.hpp"
#include "ic/InlineCacheTable.hpp"
#include "ic/TypeFeedbackCollector.hpp"
#include <asmjit/x86.h>
#include <cstddef>
#include <cstdint>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;

    static bool protocolFastKindMatchesArity(ic::MethodProtocolFastKind kind,
                                             size_t argCount)
    {
        switch (kind)
        {
            case ic::MethodProtocolFastKind::PRIMITIVE_HASH_CODE:
                return argCount == 0;
            case ic::MethodProtocolFastKind::PRIMITIVE_EQUALS:
                return argCount == 1;
            case ic::MethodProtocolFastKind::NONE:
                return false;
        }
        return false;
    }

    static void emitMirrorBoxedPrimitivePayload(JitEmissionState& s, int stackIdx)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        Gp slotAddr = cc.new_gp64();
        cc.lea(slotAddr, Mem(s.boxedBase, static_cast<int32_t>(stackIdx * valueSize)));
        Gp payload = cc.new_gp64();
        cc.mov(payload, qword_ptr(slotAddr, 8));
        cc.mov(Mem(s.stackBase, stackIdx * 8), payload);
    }

    static bool emitProtocolFastMethodCallMono(
        JitEmissionState& s,
        const bytecode::BytecodeProgram::Instruction& instr,
        const ic::MethodICEntry& entry)
    {
        const size_t argCount = instr.inlineOperands[1];
        const auto kind = entry.protocolFastKind;
        if (!protocolFastKindMatchesArity(kind, argCount)) return false;
        if (!entry.shape) return false;

        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        const int receiverIdx = s.stackDepth - static_cast<int>(argCount) - 1;
        if (receiverIdx < 0) return false;

        Label slowLabel = cc.new_label();
        Label joinLabel = cc.new_label();

        emitInlineShapeGuard(s, receiverIdx, entry.shape, slowLabel);

        Gp receiverAddr = cc.new_gp64();
        cc.lea(receiverAddr, Mem(s.boxedBase,
                                 static_cast<int32_t>(receiverIdx * valueSize)));
        Gp retReg = cc.new_gp64();

        if (kind == ic::MethodProtocolFastKind::PRIMITIVE_HASH_CODE)
        {
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_try_primitive_protocol_hash),
                      FuncSignature::build<bool, value::Value*, const value::Value*>());
            inv->set_arg(0, receiverAddr);
            inv->set_arg(1, receiverAddr);
            inv->set_ret(0, retReg);
        }
        else if (kind == ic::MethodProtocolFastKind::PRIMITIVE_EQUALS)
        {
            const int argIdx = receiverIdx + 1;
            Gp argAddr = cc.new_gp64();
            cc.lea(argAddr, Mem(s.boxedBase, static_cast<int32_t>(argIdx * valueSize)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_try_primitive_protocol_equals),
                      FuncSignature::build<bool, value::Value*, const value::Value*,
                          const value::Value*>());
            inv->set_arg(0, receiverAddr);
            inv->set_arg(1, receiverAddr);
            inv->set_arg(2, argAddr);
            inv->set_ret(0, retReg);
        }
        else
        {
            return false;
        }

        cc.test(retReg.r8(), retReg.r8());
        cc.jz(slowLabel);

        emitMirrorBoxedPrimitivePayload(s, receiverIdx);
        if (kind == ic::MethodProtocolFastKind::PRIMITIVE_EQUALS)
            emitValueDestroy(s, receiverIdx + 1);
        cc.jmp(joinLabel);

        cc.bind(slowLabel);
        emitCallMethodOpGeneric(s, instr);

        cc.bind(joinLabel);
        return true;
    }

    bool tryEmitProtocolFastMethodCall(
        JitEmissionState& s,
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (!s.typeFeedback) return false;
        if (!s.usesBoxedTypes) return false;
        if (!s.typeFeedback->getICTable().hasMethodIC(s.currentIP)) return false;

        auto& cache = s.typeFeedback->getICTable().getMethodIC(s.currentIP);
        if (cache.state != ic::ICState::MONOMORPHIC || cache.entryCount != 1)
            return false;

        const auto& entry = cache.entries[0];
        if (entry.protocolFastKind == ic::MethodProtocolFastKind::NONE)
            return false;

        return emitProtocolFastMethodCallMono(s, instr, entry);
    }
}
