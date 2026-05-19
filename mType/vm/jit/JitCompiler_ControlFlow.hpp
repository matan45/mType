#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include "JitEmissionState.hpp"
#include "../bytecode/BytecodeProgram.hpp"

namespace vm::jit
{
    // Locals (defined in JitCompiler_Locals.cpp). Called from the
    // emitControlFlowOps dispatch switch in JitCompiler_ControlFlow.cpp.
    void emitLoadLocal(JitEmissionState& s, size_t slot,
                       bool hasForcedType = false,
                       SlotType forcedType = SlotType::INT);
    void emitStoreLocal(JitEmissionState& s, size_t slot,
                        bool hasForcedType = false,
                        SlotType forcedType = SlotType::INT);
    void emitLoadVar(JitEmissionState& s, uint32_t nameIndex);
    void emitStoreVar(JitEmissionState& s, uint32_t nameIndex);
    void emitDeclareVar(JitEmissionState& s,
                        const bytecode::BytecodeProgram::Instruction& instr);

    // Returns (defined in JitCompiler_Returns.cpp).
    void emitReturnValueOp(JitEmissionState& s, const ExitHandler& onExit);
    void emitCallReturnValue(JitEmissionState& s,
                              const std::string& returnType, bool isPrimReturn);

    // Calls (defined in JitCompiler_Calls.cpp).
    bool emitCallOp(JitEmissionState& s,
                    const bytecode::BytecodeProgram::Instruction& instr);
    bool emitCallFastOp(JitEmissionState& s,
                        const bytecode::BytecodeProgram::Instruction& instr);

    // Inline GC poll emitter. Used by JUMP_BACK in the dispatch TU and by
    // tryEmitSelfTailCall in JitCompiler_Calls.cpp. Defined in
    // JitCompiler_ControlFlow.cpp.
    void emitInlineGcSafepoint(JitEmissionState& s);

    // Defined in JitCompiler_ObjectCallInline.cpp. Speculatively pastes the
    // callee's bytecode body inline if it passes the plain-function inline
    // eligibility. No runtime identity guard — invalidation is eager via
    // JitCodeCache reverse edges (registerInlineEdge / invalidatedInlineCallersOf).
    bool tryEmitInlinedFunctionCall(
        JitEmissionState& s,
        const bytecode::BytecodeProgram::FunctionMetadata* callee,
        size_t argCount,
        bytecode::FunctionNameHandle calleeHandle);
}
