#include "JitCodeCache.hpp"
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

    void JitCodeCache::invalidate(const std::string& functionName)
    {
        auto it = cache.find(functionName);
        if (it != cache.end())
        {
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
        }
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
