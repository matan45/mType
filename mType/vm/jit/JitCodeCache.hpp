#pragma once
#include <algorithm>
#include <string>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <memory>
#include "../../value/ValueType.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "JitContext.hpp"
#include "JitIdentity.hpp"
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
     * Stores JIT-compiled native code, keyed by owning program + function name.
     * Owns the asmjit JitRuntime which manages executable memory pages.
     */
    class JitCodeCache
    {
    public:
        static constexpr size_t DEFAULT_MAX_CODE_BYTES = 64 * 1024 * 1024;

        explicit JitCodeCache(size_t maxCodeBytes = DEFAULT_MAX_CODE_BYTES);
        ~JitCodeCache();

        // Look up a JIT-compiled function. Returns nullptr if not compiled.
        JitFunction lookup(bytecode::ProgramId programId,
                           const std::string& functionName) const;
        JitFunction lookup(const FunctionId& function) const;

        // Phase 2: index-based lookup used by jit_call_function_fast on the
        // CALL_FAST hot path. Returns {nullptr, INVALID_FN_HANDLE} when the
        // slot is not populated; callers fall back to name-based dispatch.
        JitIndexedEntry lookupByIndex(bytecode::ProgramId programId,
                                      size_t funcIndex) const
        {
            auto programIt = byIndex.find(programId);
            if (programIt == byIndex.end()
                || funcIndex >= programIt->second.size()) return {};
            return programIt->second[funcIndex];
        }

        // Store a compiled function. The code is already added to the JitRuntime.
        // Returns false when the function already has a live entry. Native
        // code cannot be replaced in place: ICs and index slots may retain
        // the existing pointer and require coordinated invalidation first.
        // A distinct rejected code pointer is released before returning.
        bool store(bytecode::ProgramId programId,
                   const std::string& functionName,
                   JitFunction code, size_t codeBytes = 0);

        // Compilation is refused before executable memory is allocated when
        // the cache would exceed its budget. Evicting arbitrary entries is
        // unsafe because inline caches can hold direct native-code pointers;
        // coordinated invalidation remains the only release path.
        bool canReserve(size_t codeBytes) const noexcept
        {
            return codeBytes <= maxCodeBytes - std::min(liveCodeBytes, maxCodeBytes);
        }

        // Phase 2: populate the index-keyed slot alongside the name-keyed
        // hashmap. frameName should be pre-interned on the owning program
        // (BytecodeProgram::internFrameName). Safe to call with
        // SIZE_MAX / INVALID_FN_HANDLE for OSR-style keys that aren't
        // addressable by function index.
        void storeByIndex(bytecode::ProgramId programId,
                          size_t funcIndex, JitFunction code,
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
        JitFunction invalidate(const FunctionId& function);

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
        void registerInlineEdge(bytecode::ProgramId calleeProgramId,
                                bytecode::FunctionNameHandle callee,
                                FunctionId caller);
        std::vector<FunctionId> invalidatedInlineCallersOf(
            bytecode::ProgramId calleeProgramId,
            bytecode::FunctionNameHandle callee);

        // Check if a function has been compiled
        bool contains(bytecode::ProgramId programId,
                      const std::string& functionName) const;
        bool contains(const FunctionId& function) const;

        // Get the asmjit runtime for code generation
        asmjit::JitRuntime& getRuntime() { return runtime; }

        // Get number of compiled functions
        size_t size() const { return cache.size(); }

        size_t byteSize() const noexcept { return liveCodeBytes; }
        size_t byteBudget() const noexcept { return maxCodeBytes; }
        size_t getBudgetRejectCount() const noexcept { return budgetRejectCount; }
        void recordBudgetReject() noexcept { ++budgetRejectCount; }

        // Clear all compiled code
        void clear();

    private:
        asmjit::JitRuntime runtime;
        std::unordered_map<FunctionId, JitFunction,
                           FunctionIdHash, FunctionIdEqual> cache;
        std::unordered_map<FunctionId, size_t,
                           FunctionIdHash, FunctionIdEqual> codeSizes;
        size_t liveCodeBytes = 0;
        size_t maxCodeBytes = DEFAULT_MAX_CODE_BYTES;
        size_t budgetRejectCount = 0;
        std::unordered_map<bytecode::ProgramId,
                           std::vector<JitIndexedEntry>,
                           bytecode::ProgramIdHash> byIndex;

        // MYT-316: callee handle (.id) → list of caller function names that
        // pasted the callee's body inline. Read by invalidatedInlineCallersOf
        // on redefinition; written by registerInlineEdge during JIT emission.
        std::unordered_map<InlineCalleeId,
                           std::vector<FunctionId>,
                           InlineCalleeIdHash> inlineCallers;
    };
}
