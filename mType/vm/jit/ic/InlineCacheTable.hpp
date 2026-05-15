#pragma once
#include <unordered_map>
#include <cstddef>
#include "InlineCacheTypes.hpp"

namespace vm::jit::ic
{
    class InlineCacheTable
    {
    public:
        InlineCacheTable() = default;

        // Field IC access
        FieldInlineCache& getFieldIC(size_t instructionOffset);
        bool hasFieldIC(size_t instructionOffset) const;

        // Method IC access
        MethodInlineCache& getMethodIC(size_t instructionOffset);
        bool hasMethodIC(size_t instructionOffset) const;

        // Type feedback access
        TypeFeedback& getTypeFeedback(size_t instructionOffset);
        bool hasTypeFeedback(size_t instructionOffset) const;

        // Invalidate all caches (e.g., on class redefinition)
        void invalidateAll();

        // MYT-315: zero every MethodICEntry.cachedJit that points to `evictedJit`.
        // Must be called whenever JitCodeCache::invalidate releases a
        // JitFunction's native code page — failing to do so leaves IC entries
        // holding dangling function pointers that the JIT direct-call emitter
        // would call into. The argument is `const void*` because MethodICEntry
        // stores the pointer opaquely; the caller passes the raw JitFunction
        // value reinterpret-cast to `const void*`.
        void clearCachedJitForFunction(const void* evictedJit);

        // Clear everything
        void clear();

    private:
        std::unordered_map<size_t, FieldInlineCache> fieldCaches;
        std::unordered_map<size_t, MethodInlineCache> methodCaches;
        std::unordered_map<size_t, TypeFeedback> typeFeedbackMap;
    };
}
