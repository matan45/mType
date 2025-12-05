#include "GCCoordinator.hpp"

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
        referenceVisitor = std::move(visitor);
        detector->setReferenceVisitor(visitor);
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
        // Check allocation threshold
        if (GCTracker::getInstance().getAllocationCount() >= config::ALLOCATION_THRESHOLD)
        {
            return true;
        }

        // Check suspect buffer threshold
        if (suspects->shouldTriggerCollection())
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

        try
        {
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

            // Reset allocation count
            GCTracker::getInstance().resetAllocationCount();

            // Cleanup dead objects (shared_ptr expired)
            GCTracker::getInstance().cleanupDeadObjects();
        }
        catch (...)
        {
            // Ensure flag is reset even on exception
            collectionInProgress = false;
            throw;
        }

        collectionInProgress = false;
    }
}
