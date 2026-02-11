#pragma once
#include <unordered_map>
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

        // Clear everything
        void clear();

    private:
        std::unordered_map<size_t, FieldInlineCache> fieldCaches;
        std::unordered_map<size_t, MethodInlineCache> methodCaches;
        std::unordered_map<size_t, TypeFeedback> typeFeedbackMap;
    };
}
