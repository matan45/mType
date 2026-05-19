#pragma once
#include <cstddef>
#include <unordered_map>
#include <vector>
#include "JitEmissionState.hpp"
#include "ic/InlineCacheTable.hpp"
#include "../bytecode/BytecodeProgram.hpp"

namespace vm::jit
{
    // Captures the JIT emitter state that inline emission mutates while walking
    // the callee body. Used by emitInlinedMethodCallMono / emitInlinedMethodCallPoly
    // (in JitCompiler_ObjectCallMethod.cpp) and the function-call inliner
    // (JitCompiler_ObjectCallInline.cpp) to rewind before emitting the fallback
    // slow path. currentIP must round-trip — it's baked into the IC lookup key
    // emitted by emitCallMethodOpGeneric.
    struct InlineEmitStateSnapshot
    {
        int stackDepth;
        std::vector<SlotType> slotTypes;
        std::unordered_map<size_t, SlotType> localTypes;
        std::unordered_map<int, JitEmissionState::CachedArrayInfo> arrayInfoCache;
        size_t currentIP;
    };

    // Defined in JitCompiler_ObjectCallInline.cpp. Walks the callee bytecode
    // and returns the peak operand-stack depth. Inline guards use this to
    // reject candidates whose caller_depth + callee_peak would exceed
    // MAX_OP_STACK and overrun cc.new_stack. Conservative +1 net push for
    // opcodes DataFlowAnalyzer doesn't classify (CALL_METHOD, NEW_INSTANCE,
    // iterator ops, etc.). Returns MAX_OP_STACK+1 on internal failure so
    // every caller rejects the candidate cleanly.
    size_t computeCalleePeakOperandStack(
        const bytecode::BytecodeProgram& program,
        const bytecode::BytecodeProgram::FunctionMetadata& callee);

    // Defined in JitCompiler_ObjectCallInline.cpp. Shared bytecode-paste loop;
    // pushes/pops the inlined-callee owner class onto ctx->inlinedCallingClassNames
    // for private/protected field-access validation. Caller-supplied onExit fires
    // on every RETURN_VALUE before the jmp to endLabel.
    void emitInlineCalleeBody(
        JitEmissionState& s,
        const bytecode::BytecodeProgram::FunctionMetadata& callee,
        int stackBaselineIdx,
        const ExitHandler& onExit);

    InlineEmitStateSnapshot snapshotEmitStateForInline(const JitEmissionState& s);
    void restoreEmitStateForInline(JitEmissionState& s,
                                    const InlineEmitStateSnapshot& snap);
    void normalizeInlineReturnJoinState(JitEmissionState& s,
                                         int resultStackIdx,
                                         size_t callSiteIP);

    // Defined in JitCompiler_ObjectCallMethod.cpp. The generic CALL_METHOD slow
    // path (marshal receiver+args → jit_call_method_ic → copy return). Used by
    // emitInlinedMethodCallMono/Poly's slow-path branch, the protocol-fast
    // emitter's slow path, and tryEmitDirectMethodCall's slow path.
    void emitCallMethodOpGeneric(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr);

    // Defined in JitCompiler_ObjectCallMethod.cpp. Looks up the cached JIT
    // pointer for an IC entry, falling back to a fresh JitCodeCache probe if
    // the slot is null (callee tiered up after IC fill). Returns nullptr for
    // value-class receivers or unresolved entries.
    const void* resolveCachedJit(JitEmissionState& s,
                                  const ic::MethodICEntry& entry);

    // Defined in JitCompiler_ObjectCallMethod.cpp. Shared marshal+cleanup
    // sequence for direct-dispatch sites. Mirrors emitCallMethodOpGeneric's
    // first half (copy receiver, box args, pop+destroy) without the cc.invoke.
    void emitMethodCallMarshalAndCleanup(JitEmissionState& s, size_t argCount);

    // Per-opcode emitters routed by emitObjectOps in JitCompiler_Objects.cpp.
    // Each is defined in the indicated TU.

    // JitCompiler_ObjectFields.cpp
    bool emitPushStringOp(JitEmissionState& s,
                           const bytecode::BytecodeProgram::Instruction& instr);
    bool emitGetFieldOp(JitEmissionState& s,
                        const bytecode::BytecodeProgram::Instruction& instr);
    bool emitSetFieldOp(JitEmissionState& s,
                        const bytecode::BytecodeProgram::Instruction& instr);

    // JitCompiler_ObjectCallMethod.cpp
    bool emitCallStaticOp(JitEmissionState& s,
                          const bytecode::BytecodeProgram::Instruction& instr);
    bool emitCallMethodOp(JitEmissionState& s,
                          const bytecode::BytecodeProgram::Instruction& instr);

    // JitCompiler_ObjectCallInline.cpp
    bool tryEmitInlinedStaticCall(JitEmissionState& s,
                                   const bytecode::BytecodeProgram::Instruction& instr);

    // JitCompiler_ObjectCallProtocol.cpp
    bool tryEmitProtocolFastMethodCall(JitEmissionState& s,
                                        const bytecode::BytecodeProgram::Instruction& instr);

    // JitCompiler_ObjectIteration.cpp
    bool emitGetIteratorOp(JitEmissionState& s);
    bool emitIteratorHasNextOp(JitEmissionState& s);
    bool emitIteratorNextOp(JitEmissionState& s);
    bool emitIteratorCloseOp(JitEmissionState& s);
    bool emitObjectToValueOp(JitEmissionState& s);

    // JitCompiler_ObjectTypes.cpp
    bool emitInstanceofOp(JitEmissionState& s,
                           const bytecode::BytecodeProgram::Instruction& instr);
    bool emitInstanceofTypeParamOp(JitEmissionState& s,
                                    const bytecode::BytecodeProgram::Instruction& instr);
    bool emitBindTypeArgsOp(JitEmissionState& s,
                             const bytecode::BytecodeProgram::Instruction& instr);
    bool emitCastTypeParamOp(JitEmissionState& s,
                              const bytecode::BytecodeProgram::Instruction& instr);
    bool emitCastOp(JitEmissionState& s,
                     const bytecode::BytecodeProgram::Instruction& instr);
    bool emitStructHashIntOp(JitEmissionState& s,
                              const bytecode::BytecodeProgram::Instruction& instr);
    bool emitStructEqIntOp(JitEmissionState& s,
                            const bytecode::BytecodeProgram::Instruction& instr);

    // JitCompiler_ObjectInstantiation.cpp
    bool emitNewObjectOp(JitEmissionState& s,
                          const bytecode::BytecodeProgram::Instruction& instr);
    bool emitNewStackOp(JitEmissionState& s,
                         const bytecode::BytecodeProgram::Instruction& instr);
    bool emitCreatePromiseOp(JitEmissionState& s);
    bool emitObjectToValueCreatePromiseOp(JitEmissionState& s);
    bool emitAwaitOp(JitEmissionState& s);
}
