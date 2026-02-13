#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "../../value/ValueType.hpp"
#include "JitContext.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    // Signature of a JIT-compiled function:
    // Takes a JitContext pointer, returns a Value via the context's stack
    using JitFunction = void(*)(JitContext*);

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

        // Store a compiled function. The code is already added to the JitRuntime.
        void store(const std::string& functionName, JitFunction code);

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
    };
}
