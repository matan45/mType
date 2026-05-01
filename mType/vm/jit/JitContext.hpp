#pragma once
#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
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

        // MYT-251: per-inlined-frame caller-class override. When the JIT
        // inlines `Foo::bar`'s body inside another function's compilation,
        // private/protected field accesses inside that body must be
        // validated against `Foo`, not the outer function's class. The
        // inlined-emit path pushes the callee's owner class at body
        // entry and pops at body exit; the field-access helpers prefer
        // the top-of-stack name (when depth > 0) over `callingClassName`.
        //
        // Layout chosen so the JIT can emit push/pop as a few inline
        // mov/inc/dec instructions rather than via cc.invoke — a
        // function call per inlined body invocation regressed
        // object_alloc_nested.mt and inline_value_object_hot.mt
        // measurably. The depth bound is INLINE_DEPTH_LIMIT (currently
        // 2) plus headroom; overflow at runtime is undefined but not
        // reachable while INLINE_DEPTH_LIMIT stays << capacity.
        // C-string pointers are stable for the lifetime of the JIT
        // process (interned in a static deque<string>; see
        // JitCompiler_Objects.cpp::internInlinedClassName).
        static constexpr size_t MAX_INLINED_CLASS_DEPTH = 8;
        const char* inlinedCallingClassNames[MAX_INLINED_CLASS_DEPTH] = {};
        uint64_t inlinedCallingClassDepth = 0;

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
