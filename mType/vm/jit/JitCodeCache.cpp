#include "JitCodeCache.hpp"
#include <cstddef>
#include <cstdint>

namespace vm::jit
{
    JitCodeCache::JitCodeCache()
    {
    }

    JitCodeCache::~JitCodeCache()
    {
        clear();
    }

    JitFunction JitCodeCache::lookup(const std::string& functionName) const
    {
        auto it = cache.find(functionName);
        return it != cache.end() ? it->second : nullptr;
    }

    void JitCodeCache::store(const std::string& functionName, JitFunction code)
    {
        cache[functionName] = code;
    }

    void JitCodeCache::storeByIndex(size_t funcIndex, JitFunction code,
                                    bytecode::FunctionNameHandle frameName)
    {
        if (funcIndex == SIZE_MAX) return;
        if (funcIndex >= byIndex.size()) byIndex.resize(funcIndex + 1);
        byIndex[funcIndex] = JitIndexedEntry{code, frameName};
    }

    JitFunction JitCodeCache::invalidate(const std::string& functionName)
    {
        auto it = cache.find(functionName);
        if (it == cache.end()) return nullptr;

        JitFunction removed = it->second;
        // Release the native code memory
        runtime.release(reinterpret_cast<void*>(removed));
        cache.erase(it);

        // Phase 2: also clear any matching index slot so stale pointers
        // aren't served from the fast path. Linear scan — invalidate is
        // only called on deopt, a rare event.
        for (auto& entry : byIndex)
        {
            if (entry.fn == removed)
            {
                entry = {};
                break;
            }
        }

        // MYT-316: this function may itself have been inlined into other
        // JIT'd callers. Their reverse edges are addressed by a separate
        // invalidatedInlineCallersOf(handle) call from the redefinition
        // path — we don't chain here because we don't have the handle.
        return removed;
    }

    void JitCodeCache::registerInlineEdge(
        bytecode::FunctionNameHandle callee, const std::string& caller)
    {
        if (callee == bytecode::INVALID_FN_HANDLE) return;
        if (caller.empty()) return;
        auto& vec = inlineCallers[callee.id];
        for (const auto& existing : vec)
        {
            if (existing == caller) return;
        }
        vec.push_back(caller);
    }

    std::vector<std::string> JitCodeCache::invalidatedInlineCallersOf(
        bytecode::FunctionNameHandle callee)
    {
        if (callee == bytecode::INVALID_FN_HANDLE) return {};
        auto it = inlineCallers.find(callee.id);
        if (it == inlineCallers.end()) return {};
        std::vector<std::string> result = std::move(it->second);
        inlineCallers.erase(it);
        return result;
    }

    bool JitCodeCache::contains(const std::string& functionName) const
    {
        return cache.find(functionName) != cache.end();
    }

    void JitCodeCache::clear()
    {
        for (auto& [name, code] : cache)
        {
            runtime.release(reinterpret_cast<void*>(code));
        }
        cache.clear();
        byIndex.clear();
    }
}
