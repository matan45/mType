#pragma once

#include <cstddef>
#include "JitEmissionState.hpp"

namespace vm::bytecode { class BytecodeProgram; }

namespace vm::jit
{
    // Defined in JitCompiler_OSR_Deopt.cpp. Called by JitCompiler_OSR.cpp's
    // emitOSRBody to write the JIT-side locals back into ctx->osrOutputLocals
    // at OSR exit.
    void emitLocalsWriteBack(JitEmissionState& s);

    // Hoist loop-invariant local loads into virtregs at OSR-prologue end
    // (MYT-211). Currently dead code — the call site in emitOSRBody is
    // commented out pending a fix to virtreg liveness across the
    // prologue→loop-body cc.jmp. Kept for re-enable per the comment in
    // emitOSRBody.
    void hoistInvariantLocals(JitEmissionState& s,
                              const bytecode::BytecodeProgram& program,
                              size_t loopStartOffset, size_t loopEndOffset);
}
