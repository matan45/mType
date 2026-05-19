#include "JitCompiler.hpp"
#include "JitCompiler_Shapes.hpp"
#include <cstddef>
#include <cstdint>
#include "../bytecode/OpCode.hpp"

namespace vm::jit
{
    using OpCode = bytecode::OpCode;

    JitCompiler::JitCompiler() {}

    bool JitCompiler::canCompile(const bytecode::BytecodeProgram::FunctionMetadata& meta,
                                  const bytecode::BytecodeProgram& program,
                                  OpCode* outOpcode) const
    {
        if (meta.isNative)
            return false;

        const auto& supported = getSupportedOpcodes();
        size_t endOffset = meta.startOffset + meta.instructionCount;

        // MYT-291 workaround: asmjit's cc.finalize() crashes (0xC0000005)
        // when a non-OSR function combines a regular loop (LOOP_START +
        // JUMP_BACK) with a cascading short-circuit OR/AND chain of three
        // or more JUMP_IF_*_OR_POPs. Reproduces in examples/code-editor's
        // JsonCursor::skipWs. The IR survives emitFunctionBody; the crash
        // is inside asmjit's own register allocator / assembler pass on
        // the loop back-edge. Keep this narrow so unrelated loop/call
        // functions and OSR-compiled top-level loops can still use the JIT.
        for (size_t ip = meta.startOffset; ip < endOffset; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            if (supported.find(static_cast<uint8_t>(instr.opcode)) == supported.end())
            {
                if (outOpcode) *outOpcode = instr.opcode;
                return false;
            }
        }
        if (endOffset > meta.startOffset &&
            hasUnsafeOrPopLoopShape(program, meta.startOffset, endOffset - 1))
            return false;
        // MYT-291 follow-up: JsonNode::parseObject exposed a second unsafe
        // function-level shape: a regular loop with several boxed early
        // RETURN_VALUE exits returning the same object local. Until the
        // boxed-return cleanup/codegen path is fixed, keep these functions
        // in the VM. Primitive return loops are unaffected.
        if (!isPrimitiveOrVoidReturnType(meta.returnType) &&
            endOffset > meta.startOffset &&
            hasUnsafeEarlyBoxedReturnLoopShape(program, meta.startOffset, endOffset - 1))
            return false;
        // MYT-308: heap helpers over global arrays can crash in emitted
        // function JIT code on Windows. heapPush has a dead-store sentinel
        // (`i = 0; i = -1`), while heapPop has the same global-array loop
        // shape without that sentinel. Keep these function bodies in the
        // bytecode VM while allowing ordinary local / parameter array loops
        // to compile and OSR.
        if (endOffset > meta.startOffset &&
            (hasArrayDeadStoreSentinelLoopShape(program, meta.startOffset, endOffset - 1) ||
             hasGlobalArrayLoopShape(program, meta.startOffset, endOffset - 1)))
            return false;

        return true;
    }

    bool JitCompiler::canCompileLoopOSR(size_t loopStartOffset, size_t loopEndOffset,
                                         const bytecode::BytecodeProgram& program,
                                         OpCode* outOpcode) const
    {
        const auto& supported = getSupportedOpcodes();
        for (size_t ip = loopStartOffset; ip <= loopEndOffset; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            // MYT-259: OSR-emitted RETURN/RETURN_VALUE now push the return
            // value onto the interpreter's operand stack and resume at the
            // RETURN_VALUE opcode itself (see emitReturnValueOp), so loops
            // with early-return statements compile correctly.
            // CREATE_PROMISE_RETURN_VALUE remains OSR-unsafe — its async-
            // promise wrap is fused into the opcode and the OSR-resume path
            // doesn't yet replay it; an OSR'd async function with an early
            // return still falls through. Re-enable once the OSR push
            // helper grows a promise-wrap variant.
            if (instr.opcode == OpCode::CREATE_PROMISE_RETURN_VALUE)
            {
                if (outOpcode) *outOpcode = instr.opcode;
                return false;
            }
            if (supported.find(static_cast<uint8_t>(instr.opcode)) == supported.end())
            {
                if (outOpcode) *outOpcode = instr.opcode;
                return false;
            }
        }
        // MYT-302/MYT-324: OSR's linear stack/boxed-slot model is unsafe
        // for a bool condition whose forward branch contains a helper call.
        // Emitting the skipped helper region can corrupt OSR state on
        // Windows and exit silently. Keep this forward conditional/helper-
        // call shape in the VM until OSR grows label-local stack-state
        // merge support.
        if (hasForwardConditionalCallRegion(program, loopStartOffset, loopEndOffset,
                                            outOpcode))
            return false;
        if (hasArrayDeadStoreSentinelLoopShape(program, loopStartOffset, loopEndOffset) ||
            hasGlobalArrayLoopShape(program, loopStartOffset, loopEndOffset))
        {
            if (outOpcode) *outOpcode = OpCode::STORE_LOCAL;
            return false;
        }
        return true;
    }
}
