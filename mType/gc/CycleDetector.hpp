#pragma once

#include <vector>
#include <unordered_set>
#include <functional>
#include <chrono>
#include "GCConfig.hpp"
#include "GCTracker.hpp"
#include "SuspectBuffer.hpp"

namespace gc
{
    /**
     * @brief Result of a cycle collection operation
     */
    struct CollectionResult
    {
        size_t objectsScanned = 0;
        size_t cyclesFound = 0;
        size_t objectsCollected = 0;
        std::chrono::microseconds duration{0};
        bool completed = true;  // false if aborted due to time limit
    };

    /**
     * @brief Bacon-style synchronous cycle collector
     *
     * Implements a modified version of "Concurrent Cycle Collection in
     * Reference Counted Systems" by David F. Bacon and V.T. Rajan.
     *
     * Algorithm phases:
     * 1. MARK_ROOTS: Find potential cycle roots (purple objects)
     * 2. SCAN_ROOTS: Trace from roots, coloring white objects
     * 3. COLLECT_ROOTS: Identify and collect white (garbage) objects
     *
     * This implementation is synchronous with time limits to ensure
     * pause times remain bounded.
     */
    class CycleDetector
    {
    public:
        // Function type for traversing object references
        using ReferenceVisitor = std::function<void(void* object, std::function<void(void*)> callback)>;

    private:
        GCTracker& tracker;
        SuspectBuffer& suspects;

        // Reference traversal callback (set by coordinator)
        ReferenceVisitor referenceVisitor;

        // Working sets for collection
        std::vector<void*> roots;
        std::unordered_set<void*> toFree;
        std::unordered_set<void*> visited;

        // Time limiting
        std::chrono::steady_clock::time_point startTime;
        std::chrono::milliseconds timeLimit;

    public:
        CycleDetector(GCTracker& tracker, SuspectBuffer& suspects);
        ~CycleDetector() = default;

        // Set the reference visitor callback
        void setReferenceVisitor(ReferenceVisitor visitor);

        // Main collection interface
        CollectionResult collectCycles(
            std::chrono::milliseconds maxTime = std::chrono::milliseconds(config::MAX_CYCLE_DETECTION_TIME_MS)
        );

        // Root set for external reachability check
        void setExternalRoots(const std::vector<void*>& externalRoots);

    private:
        // Bacon algorithm phases
        // markRoots returns any unprocessed suspects (on abort) for re-insertion
        std::vector<void*> markRoots(const std::vector<void*>& suspectList);
        void scanRoots();
        void collectRoots();

        // Object operations
        void markGray(void* object);
        void scan(void* object);
        void scanBlack(void* object);
        void collectWhite(void* object);

        // Helpers
        void forEachReference(void* object, std::function<void(void*)> callback);
        bool shouldAbort() const;
        void resetState();

        // External roots (from VM stack, globals, etc.)
        std::vector<void*> externalRoots;

        CycleDetector(const CycleDetector&) = delete;
        CycleDetector& operator=(const CycleDetector&) = delete;
    };
}
