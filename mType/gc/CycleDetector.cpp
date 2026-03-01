#include "CycleDetector.hpp"
#include "WriteBarrier.hpp"
#include <algorithm>
#include <stack>
#include <vector>

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

        // Extract suspects before processing
        auto suspectList = suspects.extractSuspects();

        // Phase 1: Mark potential cycle roots
        auto unprocessed = markRoots(suspectList);

        if (shouldAbort())
        {
            // Re-insert unprocessed suspects so they aren't lost
            for (void* obj : unprocessed)
            {
                suspects.addSuspect(obj);
            }
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

        // Break reference cycles by clearing all fields in collected objects
        // IMPORTANT: We must keep all objects alive while clearing to avoid
        // destroying objects while still iterating. We do this by first collecting
        // shared_ptrs to all objects, then clearing fields.

        // First, get shared_ptrs to keep all objects alive during clearing
        // Use a vector of pairs to track which objects we successfully locked
        std::vector<std::pair<void*, std::shared_ptr<void>>> liveRefs;
        liveRefs.reserve(toFree.size());
        for (void* obj : toFree)
        {
            auto sharedPtr = tracker.getSharedPtr(obj);
            if (sharedPtr)
            {
                liveRefs.push_back({obj, sharedPtr});
            }
            else
            {
                // Object already destroyed - just unregister it from tracker
                tracker.unregisterObject(obj);
            }
        }

        // Now clear all fields - ONLY on objects we successfully locked
        // This prevents use-after-free on already-destroyed objects
        for (const auto& [obj, sharedPtr] : liveRefs)
        {
            GCObjectHeader* header = tracker.getHeader(obj);
            if (header)
            {
                breakReferences(obj, header->type);
            }
        }

        // liveRefs goes out of scope here, allowing objects to be destroyed
        liveRefs.clear();

        // Unregister remaining objects (those we managed to lock above)
        // Note: Objects whose weak_ptrs expired were already unregistered above
        for (void* obj : toFree)
        {
            // Check if still registered (might have been unregistered above)
            if (tracker.getHeader(obj))
            {
                tracker.unregisterObject(obj);
            }
        }

        return result;
    }

    std::vector<void*> CycleDetector::markRoots(const std::vector<void*>& suspectList)
    {
        std::vector<void*> unprocessed;
        size_t i = 0;

        for (; i < suspectList.size(); ++i)
        {
            if (shouldAbort())
            {
                // Collect remaining unprocessed suspects
                for (size_t j = i; j < suspectList.size(); ++j)
                {
                    unprocessed.push_back(suspectList[j]);
                }
                return unprocessed;
            }

            void* obj = suspectList[i];
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

        return unprocessed;
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

        std::stack<void*> workStack;
        workStack.push(object);

        while (!workStack.empty() && workStack.size() < config::MAX_TRAVERSAL_DEPTH)
        {
            void* current = workStack.top();
            workStack.pop();

            if (!current) continue;

            GCObjectHeader* header = tracker.getHeader(current);
            if (!header) continue;

            if (header->color != config::ObjectColor::GRAY)
            {
                header->color = config::ObjectColor::GRAY;
                visited.insert(current);

                // Decrement virtual refcount of all children and queue them
                forEachReference(current, [this, &workStack](void* child) {
                    if (!child) return;

                    GCObjectHeader* childHeader = tracker.getHeader(child);
                    if (childHeader)
                    {
                        childHeader->virtualRefCount--;
                        workStack.push(child);
                    }
                });
            }
        }
    }

    void CycleDetector::scan(void* object)
    {
        if (!object) return;

        std::stack<void*> workStack;
        workStack.push(object);

        while (!workStack.empty() && workStack.size() < config::MAX_TRAVERSAL_DEPTH)
        {
            void* current = workStack.top();
            workStack.pop();

            if (!current) continue;

            GCObjectHeader* header = tracker.getHeader(current);
            if (!header) continue;

            if (header->color == config::ObjectColor::GRAY)
            {
                if (header->virtualRefCount > 0)
                {
                    // Object is reachable from outside the cycle
                    scanBlack(current);
                }
                else
                {
                    // Object is only reachable through the cycle - mark as garbage
                    header->color = config::ObjectColor::WHITE;

                    forEachReference(current, [&workStack](void* child) {
                        if (!child) return;
                        workStack.push(child);
                    });
                }
            }
        }
    }

    void CycleDetector::scanBlack(void* object)
    {
        if (!object) return;

        std::stack<void*> workStack;
        workStack.push(object);

        while (!workStack.empty() && workStack.size() < config::MAX_TRAVERSAL_DEPTH)
        {
            void* current = workStack.top();
            workStack.pop();

            if (!current) continue;

            GCObjectHeader* header = tracker.getHeader(current);
            if (!header) continue;

            if (header->color != config::ObjectColor::BLACK)
            {
                header->color = config::ObjectColor::BLACK;

                forEachReference(current, [this, &workStack](void* child) {
                    if (!child) return;

                    GCObjectHeader* childHeader = tracker.getHeader(child);
                    if (childHeader)
                    {
                        childHeader->virtualRefCount++;

                        if (childHeader->color != config::ObjectColor::BLACK)
                        {
                            workStack.push(child);
                        }
                    }
                });
            }
        }
    }

    void CycleDetector::collectWhite(void* object)
    {
        if (!object) return;

        std::stack<void*> workStack;
        workStack.push(object);

        while (!workStack.empty() && workStack.size() < config::MAX_TRAVERSAL_DEPTH)
        {
            void* current = workStack.top();
            workStack.pop();

            if (!current) continue;

            GCObjectHeader* header = tracker.getHeader(current);
            if (!header) continue;

            if (header->color == config::ObjectColor::WHITE && !header->buffered)
            {
                header->color = config::ObjectColor::BLACK;
                header->cycleCount++;

                forEachReference(current, [&workStack](void* child) {
                    if (!child) return;
                    workStack.push(child);
                });

                // Mark for collection
                toFree.insert(current);
            }
        }
    }

    void CycleDetector::forEachReference(void* object, std::function<void(void*)> callback)
    {
        if (referenceVisitor)
        {
            // CRITICAL: Only visit references if object is still alive
            // Otherwise we'd be accessing freed memory (use-after-free)
            if (tracker.isObjectAlive(object))
            {
                referenceVisitor(object, callback);
            }
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

        // Reset virtual refcounts to CURRENT refcounts for Bacon algorithm
        // The callback receives the current use_count() from weak_ptr
        tracker.forEachTrackedObject([](void* ptr, GCObjectHeader& header, long currentRefCount) {
            header.virtualRefCount = static_cast<int32_t>(currentRefCount);

            if (header.color == config::ObjectColor::WHITE ||
                header.color == config::ObjectColor::GRAY)
            {
                header.color = config::ObjectColor::BLACK;
            }
        });
    }
}
