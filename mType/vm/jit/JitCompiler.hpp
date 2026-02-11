#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "JitCodeCache.hpp"
#include "JitContext.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include <asmjit/asmjit.h>

namespace vm::jit
{
    /**
     * Baseline JIT compiler that translates mType bytecode to x86-64 native code.
     *
     * Phase 2 scope: Compiles functions containing only integer/boolean operations.
     * Functions with unsupported opcodes (object ops, lambdas, async, etc.) are
     * left to the interpreter.
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

        // Get compilation statistics
        size_t getCompileCount() const { return compileCount; }
        size_t getBailoutCount() const { return bailoutCount; }

    private:
        // Check if all opcodes in a function are supported by the JIT
        bool canCompile(const bytecode::BytecodeProgram::FunctionMetadata& meta,
                        const bytecode::BytecodeProgram& program) const;

        // Set of opcodes supported by the Phase 2 JIT
        static const std::unordered_set<uint8_t>& getSupportedOpcodes();

        size_t compileCount = 0;
        size_t bailoutCount = 0;
    };
}
