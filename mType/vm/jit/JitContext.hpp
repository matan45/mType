#pragma once
#include <cstddef>
#include <cstdint>
#include "../../value/ValueType.hpp"

// Forward declarations
namespace vm::runtime {
    class StackManager;
    struct CallFrame;
}

namespace vm::bytecode {
    class BytecodeProgram;
}

namespace environment {
    class Environment;
}

namespace vm::jit
{
    /**
     * Lightweight context passed to JIT-compiled functions.
     * Uses raw pointers to avoid shared_ptr overhead in the hot path.
     * The VM ensures these pointers remain valid for the lifetime of JIT execution.
     */
    struct JitContext
    {
        // Arguments passed to the JIT function (Value array from caller)
        const value::Value* args;
        size_t argCount;

        // Return value slot
        value::Value returnValue;
        bool hasReturnValue;

        // Program data for constant pool access
        const vm::bytecode::BytecodeProgram* program;

        // Stack manager for interop with interpreter
        vm::runtime::StackManager* stackManager;

        // Environment for global variable access and native function calls
        environment::Environment* environment;
    };
}
