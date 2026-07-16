#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>
#include "InlineCacheTypes.hpp"
#include "../JitIdentity.hpp"

namespace vm::jit::ic
{
    class InlineCacheTable
    {
    public:
        InlineCacheTable() = default;

        // Pre-size the compact site-index table when a program is bound.
        // Accessors also grow lazily so library/interop programs remain safe
        // when first observed through a nested dispatch path.
        void registerProgram(bytecode::ProgramId programId,
                             size_t instructionCount);

        FieldInlineCache& getFieldIC(bytecode::ProgramId programId,
                                     size_t instructionOffset);
        bool hasFieldIC(bytecode::ProgramId programId,
                        size_t instructionOffset) const;

        MethodInlineCache& getMethodIC(bytecode::ProgramId programId,
                                       size_t instructionOffset);
        bool hasMethodIC(bytecode::ProgramId programId,
                         size_t instructionOffset) const;

        TypeFeedback& getTypeFeedback(bytecode::ProgramId programId,
                                      size_t instructionOffset);
        bool hasTypeFeedback(bytecode::ProgramId programId,
                             size_t instructionOffset) const;

        void invalidateAll();

        // Zero every MethodICEntry.cachedJit that points to evictedJit.
        // Required before an invalidated native-code pointer can be reused.
        void clearCachedJitForFunction(const void* evictedJit);

        void clear();

    private:
        static constexpr uint32_t INVALID_CACHE_INDEX = UINT32_MAX;

        struct SiteState
        {
            uint32_t fieldIndex = INVALID_CACHE_INDEX;
            uint32_t methodIndex = INVALID_CACHE_INDEX;
            uint32_t feedbackIndex = INVALID_CACHE_INDEX;
        };

        struct ProgramSiteCaches
        {
            std::vector<SiteState> sites;

            // Deques keep cache references stable if a nested call discovers
            // another site while its caller still holds the current entry.
            std::deque<FieldInlineCache> fieldCaches;
            std::deque<MethodInlineCache> methodCaches;
            std::deque<TypeFeedback> typeFeedback;
        };

        ProgramSiteCaches& programCaches(bytecode::ProgramId programId,
                                         size_t instructionOffset);
        const ProgramSiteCaches* findProgramCaches(
            bytecode::ProgramId programId) const;

        std::unordered_map<bytecode::ProgramId,
                           std::unique_ptr<ProgramSiteCaches>,
                           bytecode::ProgramIdHash> programs;
        mutable bytecode::ProgramId activeProgramId{};
        mutable ProgramSiteCaches* activeProgram = nullptr;
    };
}
