#include "JitCodeCache.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace vm::jit
{
    JitCodeCache::JitCodeCache(size_t maxCodeBytes)
        : maxCodeBytes(maxCodeBytes)
    {
    }

    JitCodeCache::~JitCodeCache()
    {
        clear();
    }

    JitFunction JitCodeCache::lookup(bytecode::ProgramId programId,
                                     const std::string& functionName) const
    {
        auto it = cache.find(FunctionLookupId{programId, functionName});
        return it != cache.end() ? it->second : nullptr;
    }

    JitFunction JitCodeCache::lookup(const FunctionId& function) const
    {
        auto it = cache.find(function);
        return it != cache.end() ? it->second : nullptr;
    }

    bool JitCodeCache::store(bytecode::ProgramId programId,
                             const std::string& functionName,
                             JitFunction code, size_t codeBytes)
    {
        FunctionId function{programId, functionName};
        auto existing = cache.find(function);
        if (existing != cache.end())
        {
            if (code && code != existing->second)
                runtime.release(reinterpret_cast<void*>(code));
            return false;
        }

        bool cacheInserted = false;
        try
        {
            auto [entry, inserted] = cache.emplace(function, code);
            if (!inserted)
            {
                if (code && code != entry->second)
                    runtime.release(reinterpret_cast<void*>(code));
                return false;
            }
            cacheInserted = true;

            auto [sizeEntry, sizeInserted] =
                codeSizes.emplace(function, codeBytes);
            if (!sizeInserted)
            {
                cache.erase(entry);
                cacheInserted = false;
                codeSizes.erase(function);
                if (code) runtime.release(reinterpret_cast<void*>(code));
                return false;
            }
            (void)sizeEntry;
            liveCodeBytes += codeBytes;
            return true;
        }
        catch (...)
        {
            if (cacheInserted) cache.erase(function);
            if (code) runtime.release(reinterpret_cast<void*>(code));
            throw;
        }
    }

    void JitCodeCache::storeByIndex(bytecode::ProgramId programId,
                                    size_t funcIndex, JitFunction code,
                                    bytecode::FunctionNameHandle frameName)
    {
        if (funcIndex == SIZE_MAX) return;
        auto& entries = byIndex[programId];
        if (funcIndex >= entries.size()) entries.resize(funcIndex + 1);
        entries[funcIndex] = JitIndexedEntry{code, frameName};
    }

    JitFunction JitCodeCache::invalidate(const FunctionId& function)
    {
        auto it = cache.find(function);
        if (it == cache.end()) return nullptr;

        JitFunction removed = it->second;
        auto sizeIt = codeSizes.find(function);
        if (sizeIt != codeSizes.end())
        {
            liveCodeBytes -= sizeIt->second;
            codeSizes.erase(sizeIt);
        }
        // Release the native code memory
        runtime.release(reinterpret_cast<void*>(removed));
        cache.erase(it);

        // Phase 2: also clear any matching index slot so stale pointers
        // aren't served from the fast path. Linear scan — invalidate is
        // only called on deopt, a rare event.
        auto programIt = byIndex.find(function.programId);
        if (programIt != byIndex.end())
        {
            for (auto& entry : programIt->second)
            {
                if (entry.fn == removed) entry = {};
            }
        }

        // The invalidated function can also appear as a caller in reverse
        // inline-edge lists. Remove those stale references now so a later
        // callee invalidation cannot accumulate dead caller identities.
        for (auto edgeIt = inlineCallers.begin(); edgeIt != inlineCallers.end();)
        {
            auto& callers = edgeIt->second;
            callers.erase(
                std::remove(callers.begin(), callers.end(), function),
                callers.end());
            if (callers.empty()) edgeIt = inlineCallers.erase(edgeIt);
            else ++edgeIt;
        }

        // MYT-316: this function may itself have been inlined into other
        // JIT'd callers. Their reverse edges are addressed by a separate
        // invalidatedInlineCallersOf(handle) call from the redefinition
        // path — we don't chain here because we don't have the handle.
        return removed;
    }

    void JitCodeCache::registerInlineEdge(
        bytecode::ProgramId calleeProgramId,
        bytecode::FunctionNameHandle callee,
        FunctionId caller)
    {
        if (callee == bytecode::INVALID_FN_HANDLE) return;
        if (caller.name.empty()) return;
        auto& vec = inlineCallers[InlineCalleeId{calleeProgramId, callee.id}];
        for (const auto& existing : vec)
        {
            if (existing == caller) return;
        }
        vec.push_back(std::move(caller));
    }

    std::vector<FunctionId> JitCodeCache::invalidatedInlineCallersOf(
        bytecode::ProgramId calleeProgramId,
        bytecode::FunctionNameHandle callee)
    {
        if (callee == bytecode::INVALID_FN_HANDLE) return {};
        auto it = inlineCallers.find(
            InlineCalleeId{calleeProgramId, callee.id});
        if (it == inlineCallers.end()) return {};
        std::vector<FunctionId> result = std::move(it->second);
        inlineCallers.erase(it);
        return result;
    }

    bool JitCodeCache::contains(bytecode::ProgramId programId,
                                const std::string& functionName) const
    {
        return cache.find(FunctionLookupId{programId, functionName}) != cache.end();
    }

    bool JitCodeCache::contains(const FunctionId& function) const
    {
        return cache.find(function) != cache.end();
    }

    void JitCodeCache::clear()
    {
        for (auto& [function, code] : cache)
        {
            runtime.release(reinterpret_cast<void*>(code));
        }
        cache.clear();
        codeSizes.clear();
        liveCodeBytes = 0;
        budgetRejectCount = 0;
        byIndex.clear();
        inlineCallers.clear();
    }
}
