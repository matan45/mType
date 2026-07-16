#include "GCCoordinator.hpp"
#include "GCNotificationSuppression.hpp"
#include <chrono>
#include <cstddef>
#include <algorithm>
#include <limits>

namespace gc
{
    namespace
    {
        std::atomic<GCCoordinator::RootCollectorId> nextRootCollectorId{1};
    }

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
        std::lock_guard lock(rootCollectorsMutex);
        if (legacyRootCollectorId != 0)
        {
            rootCollectors.erase(legacyRootCollectorId);
            legacyRootCollectorId = 0;
        }
        if (collector)
        {
            legacyRootCollectorId = nextRootCollectorId.fetch_add(1);
            rootCollectors.emplace(legacyRootCollectorId, std::move(collector));
        }
    }

    GCCoordinator::RootCollectorId GCCoordinator::addRootCollector(RootCollector collector)
    {
        if (!collector) return 0;
        const RootCollectorId id = nextRootCollectorId.fetch_add(1);
        std::lock_guard lock(rootCollectorsMutex);
        rootCollectors.emplace(id, std::move(collector));
        return id;
    }

    void GCCoordinator::removeRootCollector(RootCollectorId id)
    {
        if (id == 0) return;
        std::lock_guard lock(rootCollectorsMutex);
        rootCollectors.erase(id);
    }

    size_t GCCoordinator::getRootCollectorCount() const
    {
        std::lock_guard lock(rootCollectorsMutex);
        return rootCollectors.size();
    }

    void GCCoordinator::setReferenceVisitor(ReferenceVisitor visitor)
    {
        std::lock_guard lock(stateMutex);
        detector->setReferenceVisitor(std::move(visitor));
    }

    void GCCoordinator::onDeallocation(void* object)
    {
        if (!object) return;
        std::lock_guard lock(stateMutex);

        // Remove from suspect buffer if present
        suspects->removeSuspect(object);

        // Unregister from tracker
        GCTracker::getInstance().unregisterObject(object);

        stats.recordDeallocation();
        stats.synchronizeTrackedObjects(
            GCTracker::getInstance().getTotalTrackedObjects());
    }

    void GCCoordinator::onRefCountDecrement(void* object)
    {
        if (!object || !enabled.load() || areReferenceNotificationsSuppressed()) return;
        std::lock_guard lock(stateMutex);
        bufferSuspectUnlocked(object);
    }

    void GCCoordinator::bufferSuspectUnlocked(void* object)
    {
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
        std::lock_guard lock(stateMutex);
        if (collectionRetryPending) return true;

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
        std::vector<RootCollector> collectorSnapshot;
        {
            std::lock_guard lock(rootCollectorsMutex);
            collectorSnapshot.reserve(rootCollectors.size());
            for (const auto& [_, collector] : rootCollectors)
            {
                collectorSnapshot.push_back(collector);
            }
        }

        // User/VM callbacks are deliberately invoked after releasing the
        // registry lock. A collector may destroy a VM or otherwise mutate the
        // registration set while roots are being assembled.
        std::vector<void*> roots;
        for (const auto& collector : collectorSnapshot)
        {
            auto collectorRoots = collector();
            roots.insert(roots.end(), collectorRoots.begin(), collectorRoots.end());
        }

        // CycleDetector keeps raw header pointers while traversing. Freeze
        // tracker registration and write-barrier bookkeeping for that phase.
        // A full concurrent mutator gate remains a separate heap-level concern.
        std::lock_guard stateLock(stateMutex);
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

        if (result.completed)
        {
            collectionRetryPending = false;
            updateAdaptiveBackoff(result.objectsCollected);

            // Only a completed pass consumes the allocation pressure that
            // triggered it. Aborted passes retain it for the retry.
            GCTracker::getInstance().resetAllocationCount();
        }
        else
        {
            collectionRetryPending = true;
        }

        // Cleanup dead objects (shared_ptr expired)
        GCTracker::getInstance().cleanupDeadObjects();
        // CycleDetector and dead-weak cleanup unregister directly from the
        // tracker. Reconcile this exposed gauge to its authoritative count.
        stats.synchronizeTrackedObjects(
            GCTracker::getInstance().getTotalTrackedObjects());
    }

    void GCCoordinator::updateAdaptiveBackoff(size_t objectsCollected)
    {
        if (objectsCollected == 0)
        {
            consecutiveEmptyCollections++;

            size_t backoffMultiplier = static_cast<size_t>(1)
                << std::min(consecutiveEmptyCollections, config::MAX_BACKOFF_EXPONENT);

            size_t trackedObjects = GCTracker::getInstance().getTotalTrackedObjects();
            size_t heapScale = std::max(static_cast<size_t>(1), trackedObjects / config::ALLOCATION_THRESHOLD);

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
            consecutiveEmptyCollections = 0;
            currentAllocationThreshold = config::ALLOCATION_THRESHOLD;
            currentSuspectThreshold = config::SUSPECT_THRESHOLD;
        }
    }

    void GCCoordinator::reset()
    {
        // Wait for any in-progress collection to complete
        while (collectionInProgress.load())
        {
            // Spin wait
        }

        std::lock_guard lock(stateMutex);

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
        collectionRetryPending = false;
    }
}
