#include "GCCoordinator.hpp"
#include <algorithm>
#include <limits>

namespace gc
{
    GCCoordinator::GCCoordinator()
        : suspects(std::make_unique<SuspectBuffer>())
        , detector(std::make_unique<CycleDetector>(GCTracker::getInstance(), *suspects))
    {
    }

    GCCoordinator::~GCCoordinator()
    {
        // Ensure no collection is in progress
        while (collectionInProgress.load())
        {
            // Spin wait - should be rare
        }
    }

    void GCCoordinator::setRootCollector(RootCollector collector)
    {
        rootCollector = std::move(collector);
    }

    void GCCoordinator::setReferenceVisitor(ReferenceVisitor visitor)
    {
        detector->setReferenceVisitor(std::move(visitor));
    }

    void GCCoordinator::onDeallocation(void* object)
    {
        if (!object) return;

        // Remove from suspect buffer if present
        suspects->removeSuspect(object);

        // Unregister from tracker
        GCTracker::getInstance().unregisterObject(object);

        stats.recordDeallocation();
    }

    void GCCoordinator::onRefCountDecrement(void* object)
    {
        if (!object || !enabled.load()) return;

        // Mark object as potential cycle root (purple)
        GCObjectHeader* header = GCTracker::getInstance().getHeader(object);
        if (header)
        {
            // Only add to suspects if not already buffered
            if (!header->buffered)
            {
                header->color = config::ObjectColor::PURPLE;
                header->buffered = true;
                suspects->addSuspect(object);
                stats.suspectsAdded++;
            }
        }
    }

    void GCCoordinator::maybeCollect()
    {
        if (!enabled.load()) return;
        if (collectionInProgress.load()) return;

        if (shouldCollect())
        {
            performCollection();
        }
    }

    void GCCoordinator::forceCollect()
    {
        if (collectionInProgress.load()) return;

        performCollection();
    }

    bool GCCoordinator::shouldCollect() const
    {
        // Check allocation threshold (adaptive — scales with heap size and backoff)
        if (GCTracker::getInstance().getAllocationCount() >= currentAllocationThreshold)
        {
            return true;
        }

        // Check suspect buffer threshold (adaptive)
        if (suspects->size() >= currentSuspectThreshold)
        {
            return true;
        }

        return false;
    }

    void GCCoordinator::performCollection()
    {
        // Set collection in progress flag
        bool expected = false;
        if (!collectionInProgress.compare_exchange_strong(expected, true))
        {
            return;  // Another collection already in progress
        }

        // RAII guard ensures flag is always reset, even during stack unwinding
        struct CollectionGuard {
            std::atomic<bool>& flag;
            CollectionGuard(std::atomic<bool>& f) : flag(f) {}
            ~CollectionGuard() { flag = false; }
        } guard(collectionInProgress);

        // Collect roots from VM
        std::vector<void*> roots;
        if (rootCollector)
        {
            roots = rootCollector();
        }
        detector->setExternalRoots(roots);

        // Run cycle detection
        auto result = detector->collectCycles(
            std::chrono::milliseconds(config::MAX_CYCLE_DETECTION_TIME_MS)
        );

        // Record statistics
        stats.recordCollection(
            result.cyclesFound,
            result.objectsCollected,
            static_cast<size_t>(result.duration.count()),
            result.completed
        );

        // Adaptive backoff: if collection found nothing, increase thresholds
        if (result.objectsCollected == 0)
        {
            consecutiveEmptyCollections++;

            // Exponential backoff: double thresholds each time, capped at 32x base
            size_t backoffMultiplier = static_cast<size_t>(1) << std::min(consecutiveEmptyCollections, static_cast<size_t>(5));

            // Scale allocation threshold based on heap size
            // Larger heaps need proportionally higher thresholds to avoid thrashing
            size_t trackedObjects = GCTracker::getInstance().getTotalTrackedObjects();
            size_t heapScale = std::max(static_cast<size_t>(1), trackedObjects / config::ALLOCATION_THRESHOLD);

            // Saturating multiplication to prevent size_t overflow
            constexpr size_t MAX_THRESHOLD = std::numeric_limits<size_t>::max() / 2;
            size_t threshold = config::ALLOCATION_THRESHOLD;
            if (backoffMultiplier > 0 && threshold <= MAX_THRESHOLD / backoffMultiplier)
                threshold *= backoffMultiplier;
            else
                threshold = MAX_THRESHOLD;
            if (heapScale > 0 && threshold <= MAX_THRESHOLD / heapScale)
                threshold *= heapScale;
            else
                threshold = MAX_THRESHOLD;
            currentAllocationThreshold = threshold;
            currentSuspectThreshold = config::SUSPECT_THRESHOLD * backoffMultiplier;
        }
        else
        {
            // Collection found garbage — reset backoff to be responsive again
            consecutiveEmptyCollections = 0;
            currentAllocationThreshold = config::ALLOCATION_THRESHOLD;
            currentSuspectThreshold = config::SUSPECT_THRESHOLD;
        }

        // Reset allocation count
        GCTracker::getInstance().resetAllocationCount();

        // Cleanup dead objects (shared_ptr expired)
        GCTracker::getInstance().cleanupDeadObjects();
    }

    void GCCoordinator::reset()
    {
        // Wait for any in-progress collection to complete
        while (collectionInProgress.load())
        {
            // Spin wait
        }

        // Clear suspect buffer
        suspects->clear();

        // Reset tracker
        GCTracker::getInstance().reset();

        // Reset stats
        stats.reset();

        // Reset flags
        collectionInProgress = false;
        enabled = true;

        // Reset adaptive backoff
        consecutiveEmptyCollections = 0;
        currentAllocationThreshold = config::ALLOCATION_THRESHOLD;
        currentSuspectThreshold = config::SUSPECT_THRESHOLD;
    }
}
