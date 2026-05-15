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

        // Invalidate a compiled function (for deoptimization).
        //
        // MYT-315 contract: any caller MUST also call
        // InlineCacheTable::clearCachedJitForFunction(removed) on every IC
        // table that may hold a reference (today: VirtualMachine::getInlineCacheTable()).
        // Skipping that leaves MethodICEntry.cachedJit slots pointing at code
        // that has already been released, and the JIT direct-call emitter
        // (emitCallMethodOpGeneric) will call into freed memory on the next
        // dispatch. JitCodeCache itself can't do the scrub because it doesn't
        // hold an IC table reference (intentional — keeps the layering one-way:
        // VM -> JitCodeCache, not vice versa).
        //
        // Returns the released JitFunction pointer so the caller can pass it
        // to clearCachedJitForFunction. Returns nullptr if `functionName` was
        // not in the cache.
        JitFunction invalidate(const std::string& functionName);

        // MYT-316: reverse caller→callee inline edges. When the JIT
        // speculatively pastes plain-function callee G's body into caller F's
        // JIT'd buffer, it calls registerInlineEdge(G, F). If G's body
        // subsequently changes (plugin reload, REPL rebinding), the runtime
        // calls invalidatedInlineCallersOf(G) to get the list of F's that
        // need their JIT'd buffers evicted.
        //
        // The map is keyed by the callee's FunctionNameHandle.id (uint32_t)
        // rather than the handle itself so we don't need to specialize
        // std::hash for FunctionNameHandle.
        //
        // Identity safety: identity is *not* guarded at runtime — the inlined
        // body is statically pasted with no shape/identity check. Safety
        // comes from eager eviction here. Pattern modeled on
        // BytecodeProgram::clearNativeCacheSlots.
        void registerInlineEdge(bytecode::FunctionNameHandle callee,
                                const std::string& caller);
        std::vector<std::string> invalidatedInlineCallersOf(
            bytecode::FunctionNameHandle callee);

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

        // MYT-316: callee handle (.id) → list of caller function names that
        // pasted the callee's body inline. Read by invalidatedInlineCallersOf
        // on redefinition; written by registerInlineEdge during JIT emission.
        std::unordered_map<uint32_t, std::vector<std::string>> inlineCallers;
    };
}
