#include "InlineCacheTable.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace vm::jit::ic
{
    namespace
    {
        uint32_t checkedCacheIndex(size_t size)
        {
            if (size >= static_cast<size_t>(UINT32_MAX))
            {
                throw std::length_error(
                    "inline cache site index exceeds uint32_t");
            }
            return static_cast<uint32_t>(size);
        }
    }

    void InlineCacheTable::registerProgram(bytecode::ProgramId programId,
                                           size_t instructionCount)
    {
        if (instructionCount == 0)
        {
            (void)programCaches(programId, 0);
            activeProgram->sites.clear();
            return;
        }
        (void)programCaches(programId, instructionCount - 1);
    }

    InlineCacheTable::ProgramSiteCaches& InlineCacheTable::programCaches(
        bytecode::ProgramId programId, size_t instructionOffset)
    {
        ProgramSiteCaches* cache = nullptr;
        if (activeProgram && activeProgramId == programId)
        {
            cache = activeProgram;
        }
        else
        {
            auto [it, inserted] = programs.try_emplace(programId);
            if (inserted) it->second = std::make_unique<ProgramSiteCaches>();
            cache = it->second.get();
            activeProgramId = programId;
            activeProgram = cache;
        }

        if (instructionOffset >= cache->sites.size())
        {
            cache->sites.resize(instructionOffset + 1);
        }
        return *cache;
    }

    const InlineCacheTable::ProgramSiteCaches*
    InlineCacheTable::findProgramCaches(bytecode::ProgramId programId) const
    {
        if (activeProgram && activeProgramId == programId) return activeProgram;
        auto it = programs.find(programId);
        if (it == programs.end()) return nullptr;
        activeProgramId = programId;
        activeProgram = it->second.get();
        return activeProgram;
    }

    FieldInlineCache& InlineCacheTable::getFieldIC(
        bytecode::ProgramId programId, size_t instructionOffset)
    {
        auto& cache = programCaches(programId, instructionOffset);
        auto& site = cache.sites[instructionOffset];
        if (site.fieldIndex == INVALID_CACHE_INDEX)
        {
            site.fieldIndex = checkedCacheIndex(cache.fieldCaches.size());
            cache.fieldCaches.emplace_back();
        }
        return cache.fieldCaches[site.fieldIndex];
    }

    bool InlineCacheTable::hasFieldIC(
        bytecode::ProgramId programId, size_t instructionOffset) const
    {
        const auto* cache = findProgramCaches(programId);
        return cache && instructionOffset < cache->sites.size()
            && cache->sites[instructionOffset].fieldIndex != INVALID_CACHE_INDEX;
    }

    MethodInlineCache& InlineCacheTable::getMethodIC(
        bytecode::ProgramId programId, size_t instructionOffset)
    {
        auto& cache = programCaches(programId, instructionOffset);
        auto& site = cache.sites[instructionOffset];
        if (site.methodIndex == INVALID_CACHE_INDEX)
        {
            site.methodIndex = checkedCacheIndex(cache.methodCaches.size());
            cache.methodCaches.emplace_back();
        }
        return cache.methodCaches[site.methodIndex];
    }

    bool InlineCacheTable::hasMethodIC(
        bytecode::ProgramId programId, size_t instructionOffset) const
    {
        const auto* cache = findProgramCaches(programId);
        return cache && instructionOffset < cache->sites.size()
            && cache->sites[instructionOffset].methodIndex != INVALID_CACHE_INDEX;
    }

    TypeFeedback& InlineCacheTable::getTypeFeedback(
        bytecode::ProgramId programId, size_t instructionOffset)
    {
        auto& cache = programCaches(programId, instructionOffset);
        auto& site = cache.sites[instructionOffset];
        if (site.feedbackIndex == INVALID_CACHE_INDEX)
        {
            site.feedbackIndex = checkedCacheIndex(cache.typeFeedback.size());
            cache.typeFeedback.emplace_back();
        }
        return cache.typeFeedback[site.feedbackIndex];
    }

    bool InlineCacheTable::hasTypeFeedback(
        bytecode::ProgramId programId, size_t instructionOffset) const
    {
        const auto* cache = findProgramCaches(programId);
        return cache && instructionOffset < cache->sites.size()
            && cache->sites[instructionOffset].feedbackIndex != INVALID_CACHE_INDEX;
    }

    void InlineCacheTable::invalidateAll()
    {
        for (auto& [programId, program] : programs)
        {
            for (auto& cache : program->fieldCaches)
            {
                cache.state = ICState::UNINITIALIZED;
                cache.entryCount = 0;
            }
            for (auto& cache : program->methodCaches)
            {
                cache.state = ICState::UNINITIALIZED;
                cache.entryCount = 0;
                cache.wide.reset();
            }
            for (auto& feedback : program->typeFeedback)
            {
                feedback.leftType = ObservedType::NONE;
                feedback.rightType = ObservedType::NONE;
                feedback.executionCount = 0;
                feedback.specialized = false;
            }
        }
    }

    void InlineCacheTable::clearCachedJitForFunction(const void* evictedJit)
    {
        if (!evictedJit) return;
        for (auto& [programId, program] : programs)
        {
            for (auto& cache : program->methodCaches)
            {
                for (uint8_t i = 0; i < cache.entryCount; ++i)
                {
                    if (cache.entries[i].cachedJit == evictedJit)
                    {
                        cache.entries[i].cachedJit = nullptr;
                    }
                }
                if (cache.wide)
                    cache.wide->clearCachedJitForFunction(evictedJit);
            }
        }
    }

    void InlineCacheTable::clear()
    {
        programs.clear();
        activeProgramId = {};
        activeProgram = nullptr;
    }
}
