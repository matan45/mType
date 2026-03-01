#pragma once

#include <cstddef>
#include <chrono>
#include <atomic>
#include <iostream>

namespace gc
{
    /**
     * @brief Statistics tracking for garbage collection
     *
     * Provides detailed metrics about GC performance and behavior
     * for debugging and tuning.
     */
    struct GCStats
    {
        // Collection counts
        std::atomic<size_t> totalCollections{0};
        std::atomic<size_t> cyclesDetected{0};
        std::atomic<size_t> objectsCollected{0};
        std::atomic<size_t> abortedCollections{0};

        // Object tracking
        std::atomic<size_t> totalAllocations{0};
        std::atomic<size_t> currentTrackedObjects{0};
        std::atomic<size_t> peakTrackedObjects{0};
        std::atomic<size_t> suspectsAdded{0};

        // Timing (in microseconds for precision)
        std::atomic<size_t> totalCollectionTimeUs{0};
        std::atomic<size_t> lastCollectionTimeUs{0};
        std::atomic<size_t> maxCollectionTimeUs{0};

        // Memory estimates
        std::atomic<size_t> estimatedHeapBytes{0};
        std::atomic<size_t> bytesFreed{0};

        void reset()
        {
            totalCollections = 0;
            cyclesDetected = 0;
            objectsCollected = 0;
            abortedCollections = 0;
            totalAllocations = 0;
            currentTrackedObjects = 0;
            peakTrackedObjects = 0;
            suspectsAdded = 0;
            totalCollectionTimeUs = 0;
            lastCollectionTimeUs = 0;
            maxCollectionTimeUs = 0;
            estimatedHeapBytes = 0;
            bytesFreed = 0;
        }

        void recordCollection(size_t cycles, size_t collected, size_t durationUs, bool completed)
        {
            totalCollections++;
            cyclesDetected += cycles;
            objectsCollected += collected;
            totalCollectionTimeUs += durationUs;
            lastCollectionTimeUs = durationUs;

            if (durationUs > maxCollectionTimeUs.load())
            {
                maxCollectionTimeUs = durationUs;
            }

            if (!completed)
            {
                abortedCollections++;
            }
        }

        void recordAllocation()
        {
            totalAllocations++;
            size_t current = ++currentTrackedObjects;
            size_t peak = peakTrackedObjects.load();
            while (current > peak && !peakTrackedObjects.compare_exchange_weak(peak, current))
            {
                // CAS loop to update peak
            }
        }

        void recordDeallocation()
        {
            size_t current = currentTrackedObjects.load();
            while (current > 0 && !currentTrackedObjects.compare_exchange_weak(current, current - 1))
            {
                // CAS loop to safely decrement without underflow
            }
        }

        double getAverageCollectionTimeMs() const
        {
            size_t collections = totalCollections.load();
            if (collections == 0) return 0.0;
            return static_cast<double>(totalCollectionTimeUs.load()) / (collections * 1000.0);
        }

        double getLastCollectionTimeMs() const
        {
            return static_cast<double>(lastCollectionTimeUs.load()) / 1000.0;
        }

        double getMaxCollectionTimeMs() const
        {
            return static_cast<double>(maxCollectionTimeUs.load()) / 1000.0;
        }

        // Print statistics to stdout for debugging
        void print() const
        {
            std::cout << "\n=== GC Statistics ===\n";
            std::cout << "Total Collections: " << totalCollections.load() << "\n";
            std::cout << "Cycles Detected: " << cyclesDetected.load() << "\n";
            std::cout << "Objects Collected: " << objectsCollected.load() << "\n";
            std::cout << "Aborted Collections: " << abortedCollections.load() << "\n";
            std::cout << "Total Allocations: " << totalAllocations.load() << "\n";
            std::cout << "Current Tracked Objects: " << currentTrackedObjects.load() << "\n";
            std::cout << "Peak Tracked Objects: " << peakTrackedObjects.load() << "\n";
            std::cout << "Suspects Added: " << suspectsAdded.load() << "\n";
            std::cout << "Avg Collection Time: " << getAverageCollectionTimeMs() << " ms\n";
            std::cout << "Max Collection Time: " << getMaxCollectionTimeMs() << " ms\n";
            std::cout << "=====================\n";
        }
    };
}
