#pragma once
#include <string>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <memory>
#include "../../value/ValueType.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "JitContext.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    // Signature of a JIT-compiled function:
    // Takes a JitContext pointer, returns a Value via the context's stack
    using JitFunction = void(*)(JitContext*);

    // Phase 2: per-index cache entry — pairs the JIT-compiled function
    // pointer with a pre-interned frame-name handle so the nested-dispatch
    // hot path skips both the name hashmap and the per-call internFrameName
    // lookup.
    struct JitIndexedEntry
    {
        JitFunction fn = nullptr;
        bytecode::FunctionNameHandle frameName = bytecode::INVALID_FN_HANDLE;
    };

    /**
     * Stores JIT-compiled native code, keyed by function name.
     * Owns the asmjit JitRuntime which manages executable memory pages.
     */
    class JitCodeCache
    {
    public:
        JitCodeCache();
        ~JitCodeCache();

        // Look up a JIT-compiled function. Returns nullptr if not compiled.
        JitFunction lookup(const std::string& functionName) const;

        // Phase 2: index-based lookup used by jit_call_function_fast on the
        // CALL_FAST hot path. Returns {nullptr, INVALID_FN_HANDLE} when the
        // slot is not populated; callers fall back to name-based dispatch.
        JitIndexedEntry lookupByIndex(size_t funcIndex) const
        {
            if (funcIndex >= byIndex.size()) return {};
            return byIndex[funcIndex];
        }

        // Store a compiled function. The code is already added to the JitRuntime.
        void store(const std::string& functionName, JitFunction code);

        // Phase 2: populate the index-keyed slot alongside the name-keyed
        // hashmap. frameName should be pre-interned on the owning program
        // (BytecodeProgram::internFrameName). Safe to call with
        // SIZE_MAX / INVALID_FN_HANDLE for OSR-style keys that aren't
        // addressable by function index.
        void storeByIndex(size_t funcIndex, JitFunction code,
                          bytecode::FunctionNameHandle frameName);

        // Invalidate a compiled function (for deoptimization)
        void invalidate(const std::string& functionName);

        // Check if a function has been compiled
        bool contains(const std::string& functionName) const;

        // Get the asmjit runtime for code generation
        asmjit::JitRuntime& getRuntime() { return runtime; }

        // Get number of compiled functions
        size_t size() const { return cache.size(); }

        // Clear all compiled code
        void clear();

    private:
        asmjit::JitRuntime runtime;
        std::unordered_map<std::string, JitFunction> cache;
        std::vector<JitIndexedEntry> byIndex;
    };
}
