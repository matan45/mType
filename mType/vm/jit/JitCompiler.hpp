#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "JitCodeCache.hpp"
#include "JitContext.hpp"
#include "SlotType.hpp"
#include "OSRState.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    /**
     * JIT compiler that translates mType bytecode to x86-64 native code.
     *
     * Phase 2: Integer/boolean arithmetic only.
     * Phase 3: Adds CALL support, generic arithmetic, float ops.
     * Phase 4: Adds object, array, string, method call support.
     *
     * The compiler uses asmjit's Compiler API for virtual register allocation
     * and compile-time stack depth tracking to eliminate operand stack overhead.
     */
    class JitCompiler
    {
    public:
        JitCompiler();

        /**
         * Attempt to compile a function to native code.
         * Returns true if compilation succeeded, false if the function
         * contains unsupported opcodes and must stay on the interpreter.
         */
        bool compile(const std::string& functionName,
                     const bytecode::BytecodeProgram& program,
                     JitCodeCache& codeCache);

        size_t getCompileCount() const { return compileCount; }
        size_t getBailoutCount() const { return bailoutCount; }

        /**
         * Phase 5 (OSR): Compile a loop body for on-stack replacement.
         * Generates native code that loads locals from JitContext.osrLocals,
         * executes the loop, and writes updated locals on exit.
         * Stores in codeCache under key "osr@{jumpBackOffset}".
         */
        bool compileLoopOSR(size_t loopStartOffset,
                            size_t loopEndOffset,
                            size_t jumpBackOffset,
                            const std::vector<LocalSlotInfo>& localSlotInfos,
                            size_t localCount,
                            const bytecode::BytecodeProgram& program,
                            JitCodeCache& codeCache);

    private:
        bool canCompile(const bytecode::BytecodeProgram::FunctionMetadata& meta,
                        const bytecode::BytecodeProgram& program) const;

        bool canCompileLoopOSR(size_t loopStartOffset, size_t loopEndOffset,
                               const bytecode::BytecodeProgram& program) const;

        static const std::unordered_set<uint8_t>& getSupportedOpcodes();

        size_t compileCount = 0;
        size_t bailoutCount = 0;
    };
}
