#pragma once
#include <cstddef>
#include "../bytecode/BytecodeProgram.hpp"
#include "../bytecode/OpCode.hpp"

namespace vm::jit
{
    // Shape-detection helpers used by JitCompiler::canCompile (function-level
    // gates) and JitCompiler::canCompileLoopOSR (loop-level gates). Each
    // identifies a specific bytecode shape that the asmjit codegen / OSR
    // resume model can't handle correctly today — see callers for the
    // associated MYT tickets (MYT-291, MYT-302, MYT-308, MYT-324).
    bool hasUnsafeOrPopLoopShape(const bytecode::BytecodeProgram& program,
                                  size_t startOffset,
                                  size_t endOffsetInclusive);

    bool hasUnsafeEarlyBoxedReturnLoopShape(const bytecode::BytecodeProgram& program,
                                             size_t startOffset,
                                             size_t endOffsetInclusive);

    bool hasArrayDeadStoreSentinelLoopShape(const bytecode::BytecodeProgram& program,
                                             size_t startOffset,
                                             size_t endOffsetInclusive);

    bool hasGlobalArrayLoopShape(const bytecode::BytecodeProgram& program,
                                  size_t startOffset,
                                  size_t endOffsetInclusive);

    bool hasForwardConditionalCallRegion(const bytecode::BytecodeProgram& program,
                                          size_t startOffset,
                                          size_t endOffsetInclusive,
                                          bytecode::OpCode* outOpcode);

    bool isPrimitiveOrVoidReturnType(const std::string& type);
}
