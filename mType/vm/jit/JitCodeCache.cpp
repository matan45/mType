#include "JitCodeCache.hpp"

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

    void JitCodeCache::invalidate(const std::string& functionName)
    {
        auto it = cache.find(functionName);
        if (it != cache.end())
        {
            // Release the native code memory
            runtime.release(reinterpret_cast<void*>(it->second));
            cache.erase(it);
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
    }
}
