#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "JitCodeCache.hpp"
#include "JitContext.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    /**
     * Compile-time type of an operand stack slot.
     * Used to determine boxing/unboxing and which native instructions to emit.
     */
    enum class SlotType : uint8_t
    {
        INT,    // int64_t in GP register
        FLOAT,  // float in XMM register (stored as 32-bit in 8-byte slot)
        BOOL    // bool stored as int64_t (0 or 1)
    };

    /**
     * JIT compiler that translates mType bytecode to x86-64 native code.
     *
     * Phase 2: Integer/boolean arithmetic only.
     * Phase 3: Adds CALL support, generic arithmetic, float ops.
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

    private:
        bool canCompile(const bytecode::BytecodeProgram::FunctionMetadata& meta,
                        const bytecode::BytecodeProgram& program) const;

        static const std::unordered_set<uint8_t>& getSupportedOpcodes();

        size_t compileCount = 0;
        size_t bailoutCount = 0;
    };
}
