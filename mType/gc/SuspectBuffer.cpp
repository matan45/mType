#include "SuspectBuffer.hpp"
#include <algorithm>

namespace gc
{
    SuspectBuffer::SuspectBuffer(size_t max)
        : maxSize(max)
    {
        suspects.reserve(config::SUSPECT_BUFFER_INITIAL_SIZE);
    }

    void SuspectBuffer::addSuspect(void* object)
    {
        if (!object) return;

        // Already in buffer
        if (suspectSet.count(object) > 0)
        {
            return;
        }

        // Buffer full - don't add more (will trigger collection)
        if (suspects.size() >= maxSize)
        {
            return;
        }

        suspects.push_back(object);
        suspectSet.insert(object);
    }

    void SuspectBuffer::removeSuspect(void* object)
    {
        if (!object) return;

        if (suspectSet.erase(object) > 0)
        {
            auto it = std::find(suspects.begin(), suspects.end(), object);
            if (it != suspects.end())
            {
                // Swap with last and pop for O(1) removal
                std::iter_swap(it, suspects.end() - 1);
                suspects.pop_back();
            }
        }
    }

    bool SuspectBuffer::isSuspect(void* object) const
    {
        return suspectSet.count(object) > 0;
    }

    std::vector<void*> SuspectBuffer::extractSuspects()
    {
        std::vector<void*> result = std::move(suspects);
        suspects.clear();
        suspects.reserve(config::SUSPECT_BUFFER_INITIAL_SIZE);
        suspectSet.clear();
        return result;
    }

    std::vector<void*> SuspectBuffer::getSuspectsCopy() const
    {
        return suspects;
    }

    size_t SuspectBuffer::size() const
    {
        return suspects.size();
    }

    bool SuspectBuffer::shouldTriggerCollection() const
    {
        return suspects.size() >= config::SUSPECT_THRESHOLD;
    }

    bool SuspectBuffer::isEmpty() const
    {
        return suspects.empty();
    }

    void SuspectBuffer::clear()
    {
        suspects.clear();
        suspects.reserve(config::SUSPECT_BUFFER_INITIAL_SIZE);
        suspectSet.clear();
    }
}
