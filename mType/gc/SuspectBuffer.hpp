#pragma once

#include <vector>
#include <cstddef>
#include <unordered_set>
#include <mutex>
#include "GCConfig.hpp"

namespace gc
{
    /**
     * @brief Buffer for potential cycle roots (Bacon-style suspect list)
     *
     * Objects are added to this buffer when their reference count decreases
     * but doesn't reach zero. These are candidates for cycle detection.
     * This implements the "purple" object tracking from Bacon's algorithm.
     */
    class SuspectBuffer
    {
    private:
        std::vector<void*> suspects;
        std::unordered_set<void*> suspectSet;  // For O(1) contains check
        size_t maxSize;

    public:
        explicit SuspectBuffer(size_t max = config::SUSPECT_BUFFER_MAX_SIZE);
        ~SuspectBuffer() = default;

        // Suspect management
        void addSuspect(void* object);
        void removeSuspect(void* object);
        bool isSuspect(void* object) const;

        // Processing
        std::vector<void*> extractSuspects();
        std::vector<void*> getSuspectsCopy() const;

        // Status
        size_t size() const;
        bool shouldTriggerCollection() const;
        bool isEmpty() const;

        // Cleanup
        void clear();

    private:
        SuspectBuffer(const SuspectBuffer&) = delete;
        SuspectBuffer& operator=(const SuspectBuffer&) = delete;
    };
}
