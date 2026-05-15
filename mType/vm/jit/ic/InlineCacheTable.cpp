#include "InlineCacheTable.hpp"
#include <cstddef>

namespace vm::jit::ic
{
    FieldInlineCache& InlineCacheTable::getFieldIC(size_t instructionOffset)
    {
        return fieldCaches[instructionOffset];
    }

    bool InlineCacheTable::hasFieldIC(size_t instructionOffset) const
    {
        return fieldCaches.count(instructionOffset) > 0;
    }

    MethodInlineCache& InlineCacheTable::getMethodIC(size_t instructionOffset)
    {
        return methodCaches[instructionOffset];
    }

    bool InlineCacheTable::hasMethodIC(size_t instructionOffset) const
    {
        return methodCaches.count(instructionOffset) > 0;
    }

    TypeFeedback& InlineCacheTable::getTypeFeedback(size_t instructionOffset)
    {
        return typeFeedbackMap[instructionOffset];
    }

    bool InlineCacheTable::hasTypeFeedback(size_t instructionOffset) const
    {
        return typeFeedbackMap.count(instructionOffset) > 0;
    }

    void InlineCacheTable::invalidateAll()
    {
        for (auto& [offset, cache] : fieldCaches)
        {
            cache.state = ICState::UNINITIALIZED;
            cache.entryCount = 0;
        }
        for (auto& [offset, cache] : methodCaches)
        {
            cache.state = ICState::UNINITIALIZED;
            cache.entryCount = 0;
        }
        for (auto& [offset, feedback] : typeFeedbackMap)
        {
            feedback.leftType = ObservedType::NONE;
            feedback.rightType = ObservedType::NONE;
            feedback.executionCount = 0;
            feedback.specialized = false;
        }
    }

    void InlineCacheTable::clearCachedJitForFunction(const void* evictedJit)
    {
        if (!evictedJit) return;
        for (auto& [offset, cache] : methodCaches)
        {
            for (uint8_t i = 0; i < cache.entryCount; ++i)
            {
                if (cache.entries[i].cachedJit == evictedJit)
                {
                    cache.entries[i].cachedJit = nullptr;
                }
            }
        }
    }

    void InlineCacheTable::clear()
    {
        fieldCaches.clear();
        methodCaches.clear();
        typeFeedbackMap.clear();
    }
}
