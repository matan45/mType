#pragma once
#include <cstddef>
#include <cstdint>
#include <exception>
#include "../../value/ValueType.hpp"

// Forward declarations
namespace vm::runtime {
    class StackManager;
    class VirtualMachine;
    struct CallFrame;
}

namespace vm::bytecode {
    class BytecodeProgram;
}

namespace environment {
    class Environment;
}

namespace vm::jit::ic { class InlineCacheTable; }

namespace vm::jit
{
    class JitCodeCache;

    /**
     * Lightweight context passed to JIT-compiled functions.
     * Uses raw pointers to avoid shared_ptr overhead in the hot path.
     * The VM ensures these pointers remain valid for the lifetime of JIT execution.
     */
    struct JitContext
    {
        const value::Value* args;
        size_t argCount;

        value::Value returnValue;
        bool hasReturnValue;

        const vm::bytecode::BytecodeProgram* program;
        vm::runtime::StackManager* stackManager;
        environment::Environment* environment;
        vm::runtime::VirtualMachine* vm;
        vm::jit::JitCodeCache* jitCodeCache;

        static constexpr size_t MAX_CALL_ARGS = 16;
        value::Value callArgs[MAX_CALL_ARGS];

        vm::jit::ic::InlineCacheTable* icTable = nullptr;
        std::string callingClassName;

        value::Value* osrLocals = nullptr;
        size_t osrLocalCount = 0;
        value::Value* osrOutputLocals = nullptr;
        size_t osrExitOffset = 0;
        bool osrExited = false;

        // Exception propagation: JIT helpers catch exceptions and store them here
        // so they don't need to unwind through asmjit-generated frames (no SEH).
        // Callers must check and rethrow after JIT code returns.
        std::exception_ptr pendingException = nullptr;
    };
}
