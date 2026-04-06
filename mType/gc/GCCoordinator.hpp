#pragma once

#include <memory>
#include <atomic>
#include <functional>
#include <vector>
#include "GCTracker.hpp"
#include "SuspectBuffer.hpp"
#include "CycleDetector.hpp"
#include "GCStats.hpp"

namespace gc
{
    /**
     * @brief Main garbage collection coordinator
     *
     * Integrates with VirtualMachine to:
     * - Track allocations of GC-managed objects
     * - Trigger collections at safe points
     * - Coordinate cycle detection
     * - Provide statistics and debugging
     *
     * Usage:
     *   1. Create coordinator instance
     *   2. Set reference visitor for object traversal
     *   3. Call onAllocation() when creating GC objects
     *   4. Call onRefCountDecrement() when refs decrease
     *   5. Call maybeCollect() periodically (e.g., every N instructions)
     */
    class GCCoordinator
    {
    public:
        // Function to collect root set from VM
        using RootCollector = std::function<std::vector<void*>()>;

        // Function to visit references from an object
        using ReferenceVisitor = std::function<void(void* object, std::function<void(void*)> callback)>;

    private:
        std::unique_ptr<SuspectBuffer> suspects;
        std::unique_ptr<CycleDetector> detector;

        std::atomic<bool> collectionInProgress{false};
        std::atomic<bool> enabled{true};

        GCStats stats;

        // Adaptive backoff: when collections find nothing, back off exponentially
        size_t consecutiveEmptyCollections = 0;
        size_t currentAllocationThreshold = config::ALLOCATION_THRESHOLD;
        size_t currentSuspectThreshold = config::SUSPECT_THRESHOLD;

        // Callbacks for VM integration
        RootCollector rootCollector;

    public:
        GCCoordinator();
        ~GCCoordinator();

        // Configuration
        void setRootCollector(RootCollector collector);
        void setReferenceVisitor(ReferenceVisitor visitor);

        // Object lifecycle hooks
        template<typename T>
        void onAllocation(std::shared_ptr<T> object, config::GCObjectType type)
        {
            GCTracker::getInstance().registerObject(object, type);
            stats.recordAllocation();
        }

        void onDeallocation(void* object);
        void onRefCountDecrement(void* object);

        // Collection control
        void maybeCollect();           // Check if collection needed
        void forceCollect();           // Force immediate collection
        void enable() { enabled = true; }
        void disable() { enabled = false; }
        bool isEnabled() const { return enabled.load(); }
        bool isCollecting() const { return collectionInProgress.load(); }

        // Statistics
        const GCStats& getStats() const { return stats; }
        GCStats& getStats() { return stats; }
        void resetStats() { stats.reset(); }

        // Tracker access
        GCTracker& getTracker() { return GCTracker::getInstance(); }

        // Reset all GC state (for test isolation)
        void reset();

    private:
        void performCollection();
        bool shouldCollect() const;
        void updateAdaptiveBackoff(size_t objectsCollected);

        GCCoordinator(const GCCoordinator&) = delete;
        GCCoordinator& operator=(const GCCoordinator&) = delete;
    };
}
