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

        std::lock_guard<std::mutex> lock(bufferMutex);

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

        std::lock_guard<std::mutex> lock(bufferMutex);

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
        std::lock_guard<std::mutex> lock(bufferMutex);
        return suspectSet.count(object) > 0;
    }

    std::vector<void*> SuspectBuffer::extractSuspects()
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        std::vector<void*> result = std::move(suspects);
        suspects.clear();
        suspects.reserve(config::SUSPECT_BUFFER_INITIAL_SIZE);
        suspectSet.clear();
        return result;
    }

    std::vector<void*> SuspectBuffer::getSuspectsCopy() const
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        return suspects;
    }

    size_t SuspectBuffer::size() const
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        return suspects.size();
    }

    bool SuspectBuffer::shouldTriggerCollection() const
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        return suspects.size() >= config::SUSPECT_THRESHOLD;
    }

    bool SuspectBuffer::isEmpty() const
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        return suspects.empty();
    }

    void SuspectBuffer::clear()
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        suspects.clear();
        suspects.reserve(config::SUSPECT_BUFFER_INITIAL_SIZE);
        suspectSet.clear();
    }
}
