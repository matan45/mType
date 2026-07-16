#pragma once

#include <memory>
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <functional>
#include <mutex>
#include <unordered_map>
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
        using RootCollectorId = uint64_t;

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
        bool collectionRetryPending = false;

        // Serializes tracker/suspect mutation with cycle detection. Reference
        // notifications caused by the collector's own cycle breaking are
        // suppressed on that thread, so this lock need not be recursive.
        // Root callbacks run before this lock is held.
        mutable std::mutex stateMutex;

        // Callbacks for VM integration
        mutable std::mutex rootCollectorsMutex;
        std::unordered_map<RootCollectorId, RootCollector> rootCollectors;
        RootCollectorId legacyRootCollectorId = 0;

    public:
        GCCoordinator();
        ~GCCoordinator();

        // Configuration
        void setRootCollector(RootCollector collector);
        RootCollectorId addRootCollector(RootCollector collector);
        void removeRootCollector(RootCollectorId id);
        size_t getRootCollectorCount() const;
        void setReferenceVisitor(ReferenceVisitor visitor);

        // Object lifecycle hooks
        template<typename T>
        void onAllocation(std::shared_ptr<T> object, config::GCObjectType type)
        {
            std::lock_guard lock(stateMutex);
            if (GCTracker::getInstance().registerObject(object, type))
            {
                // A pooled address can still be present in the suspect set
                // for its expired prior occupant.
                suspects->removeSuspect(object.get());
                stats.recordAllocation();
                stats.synchronizeTrackedObjects(
                    GCTracker::getInstance().getTotalTrackedObjects());
                // A freshly registered object may already contain captures or
                // fields. Buffer it once so cycles assembled during construction
                // do not depend on a later overwrite barrier to become visible.
                bufferSuspectUnlocked(object.get());
            }
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
        void bufferSuspectUnlocked(void* object);
        void performCollection();
        bool shouldCollect() const;
        void updateAdaptiveBackoff(size_t objectsCollected);

        GCCoordinator(const GCCoordinator&) = delete;
        GCCoordinator& operator=(const GCCoordinator&) = delete;
    };
}
