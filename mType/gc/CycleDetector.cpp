#include "CycleDetector.hpp"
#include <algorithm>

namespace gc
{
    CycleDetector::CycleDetector(GCTracker& tracker, SuspectBuffer& suspects)
        : tracker(tracker)
        , suspects(suspects)
        , timeLimit(std::chrono::milliseconds(config::MAX_CYCLE_DETECTION_TIME_MS))
    {
    }

    void CycleDetector::setReferenceVisitor(ReferenceVisitor visitor)
    {
        referenceVisitor = std::move(visitor);
    }

    void CycleDetector::setExternalRoots(const std::vector<void*>& roots)
    {
        externalRoots = roots;
    }

    CollectionResult CycleDetector::collectCycles(std::chrono::milliseconds maxTime)
    {
        CollectionResult result;
        startTime = std::chrono::steady_clock::now();
        timeLimit = maxTime;

        resetState();

        // Phase 1: Mark potential cycle roots
        markRoots();

        if (shouldAbort())
        {
            result.completed = false;
            result.duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - startTime);
            return result;
        }

        // Phase 2: Scan from roots
        scanRoots();

        if (shouldAbort())
        {
            result.completed = false;
            result.duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - startTime);
            return result;
        }

        // Phase 3: Collect garbage
        collectRoots();

        result.objectsScanned = visited.size();
        result.objectsCollected = toFree.size();
        result.cyclesFound = toFree.empty() ? 0 : 1;  // Simplified: count as 1 if any cycles found
        result.completed = true;
        result.duration = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - startTime);

        // Actually break cycles by unregistering collected objects
        for (void* obj : toFree)
        {
            tracker.unregisterObject(obj);
        }

        return result;
    }

    void CycleDetector::markRoots()
    {
        // Get suspects and mark them as potential cycle roots
        auto suspectList = suspects.extractSuspects();

        for (void* obj : suspectList)
        {
            if (shouldAbort()) return;

            GCObjectHeader* header = tracker.getHeader(obj);
            if (!header) continue;

            // Only process purple (potential cycle root) objects
            if (header->color == config::ObjectColor::PURPLE)
            {
                markGray(obj);
                roots.push_back(obj);
            }
            else
            {
                header->buffered = false;

                // If black and has no references, it's garbage
                if (header->color == config::ObjectColor::BLACK)
                {
                    // Reference count reached zero, handled by shared_ptr
                }
            }
        }

        // Mark external roots as black (definitely reachable)
        for (void* obj : externalRoots)
        {
            GCObjectHeader* header = tracker.getHeader(obj);
            if (header)
            {
                scanBlack(obj);
            }
        }
    }

    void CycleDetector::scanRoots()
    {
        for (void* root : roots)
        {
            if (shouldAbort()) return;
            scan(root);
        }
    }

    void CycleDetector::collectRoots()
    {
        for (void* root : roots)
        {
            if (shouldAbort()) return;

            GCObjectHeader* header = tracker.getHeader(root);
            if (!header) continue;

            header->buffered = false;

            collectWhite(root);
        }
    }

    void CycleDetector::markGray(void* object)
    {
        if (!object) return;

        GCObjectHeader* header = tracker.getHeader(object);
        if (!header) return;

        if (header->color != config::ObjectColor::GRAY)
        {
            header->color = config::ObjectColor::GRAY;
            visited.insert(object);

            // Decrement virtual refcount of all children
            forEachReference(object, [this](void* child) {
                if (!child) return;

                GCObjectHeader* childHeader = tracker.getHeader(child);
                if (childHeader)
                {
                    childHeader->virtualRefCount--;
                    markGray(child);
                }
            });
        }
    }

    void CycleDetector::scan(void* object)
    {
        if (!object) return;

        GCObjectHeader* header = tracker.getHeader(object);
        if (!header) return;

        if (header->color == config::ObjectColor::GRAY)
        {
            if (header->virtualRefCount > 0)
            {
                // Object is reachable from outside the cycle
                scanBlack(object);
            }
            else
            {
                // Object is only reachable through the cycle - mark as garbage
                header->color = config::ObjectColor::WHITE;

                forEachReference(object, [this](void* child) {
                    scan(child);
                });
            }
        }
    }

    void CycleDetector::scanBlack(void* object)
    {
        if (!object) return;

        GCObjectHeader* header = tracker.getHeader(object);
        if (!header) return;

        if (header->color != config::ObjectColor::BLACK)
        {
            header->color = config::ObjectColor::BLACK;

            forEachReference(object, [this](void* child) {
                if (!child) return;

                GCObjectHeader* childHeader = tracker.getHeader(child);
                if (childHeader)
                {
                    childHeader->virtualRefCount++;

                    if (childHeader->color != config::ObjectColor::BLACK)
                    {
                        scanBlack(child);
                    }
                }
            });
        }
    }

    void CycleDetector::collectWhite(void* object)
    {
        if (!object) return;

        GCObjectHeader* header = tracker.getHeader(object);
        if (!header) return;

        if (header->color == config::ObjectColor::WHITE && !header->buffered)
        {
            header->color = config::ObjectColor::BLACK;
            header->cycleCount++;

            forEachReference(object, [this](void* child) {
                collectWhite(child);
            });

            // Mark for collection
            toFree.insert(object);
        }
    }

    void CycleDetector::forEachReference(void* object, std::function<void(void*)> callback)
    {
        if (referenceVisitor)
        {
            referenceVisitor(object, callback);
        }
    }

    bool CycleDetector::shouldAbort() const
    {
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        return elapsed >= timeLimit;
    }

    void CycleDetector::resetState()
    {
        roots.clear();
        toFree.clear();
        visited.clear();

        // Reset virtual refcounts for all tracked objects
        tracker.forEachTrackedObject([](void* ptr, GCObjectHeader& header) {
            header.virtualRefCount = 0;
            if (header.color == config::ObjectColor::WHITE ||
                header.color == config::ObjectColor::GRAY)
            {
                header.color = config::ObjectColor::BLACK;
            }
        });
    }
}
