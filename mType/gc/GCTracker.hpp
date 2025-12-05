#pragma once

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <functional>
#include "GCConfig.hpp"

namespace runtimeTypes::klass { class ObjectInstance; }
namespace vm::runtime { struct BytecodeLambda; }
namespace value {
    class NativeArray;
    class FlatMultiArray;
    class SparseMultiArray;
    class PromiseValue;
}
namespace mType::value::arrays { class FlatMultiObjectArray; }

namespace gc
{
    /**
     * @brief Metadata header for GC-tracked objects
     *
     * Stored alongside each GC-managed object to track its state
     * during cycle detection.
     */
    struct GCObjectHeader
    {
        config::ObjectColor color = config::ObjectColor::BLACK;
        config::GCObjectType type = config::GCObjectType::UNKNOWN;
        bool buffered = false;           // Is this object in the suspect buffer?
        int32_t virtualRefCount = 0;     // Virtual refcount for cycle detection
        uint32_t cycleCount = 0;         // Number of times this object was in a cycle

        void reset()
        {
            color = config::ObjectColor::BLACK;
            buffered = false;
            virtualRefCount = 0;
        }
    };

    /**
     * @brief Central registry for all GC-managed objects
     *
     * Tracks all heap-allocated objects that can participate in reference cycles.
     * Uses a header map for metadata storage with minimal per-object overhead.
     */
    class GCTracker
    {
    private:
        // Object headers indexed by raw pointer
        std::unordered_map<void*, GCObjectHeader> objectHeaders;

        // Weak references to tracked objects for safe iteration
        std::unordered_map<void*, std::weak_ptr<void>> weakReferences;

        mutable std::mutex trackerMutex;
        std::atomic<size_t> allocationCount{0};
        std::atomic<size_t> totalTrackedObjects{0};

        // Singleton instance
        static GCTracker* instance;
        static std::mutex instanceMutex;

    public:
        static GCTracker& getInstance();
        static void destroyInstance();

        // Object registration
        template<typename T>
        void registerObject(std::shared_ptr<T> obj, config::GCObjectType type)
        {
            if (!obj) return;

            std::lock_guard<std::mutex> lock(trackerMutex);
            void* rawPtr = obj.get();

            if (objectHeaders.find(rawPtr) == objectHeaders.end())
            {
                GCObjectHeader header;
                header.type = type;
                header.color = config::ObjectColor::BLACK;
                objectHeaders[rawPtr] = header;
                weakReferences[rawPtr] = std::weak_ptr<void>(obj);
                totalTrackedObjects++;
                allocationCount++;
            }
        }

        void unregisterObject(void* rawPtr);

        // Header access
        GCObjectHeader* getHeader(void* rawPtr);
        const GCObjectHeader* getHeader(void* rawPtr) const;

        // Check if object is still alive (weak_ptr valid)
        bool isObjectAlive(void* rawPtr) const;

        // Statistics
        size_t getAllocationCount() const { return allocationCount.load(); }
        size_t getTotalTrackedObjects() const { return totalTrackedObjects.load(); }
        void resetAllocationCount() { allocationCount = 0; }

        // Iteration for cycle detection
        template<typename Func>
        void forEachTrackedObject(Func&& callback)
        {
            std::lock_guard<std::mutex> lock(trackerMutex);
            for (auto& [ptr, header] : objectHeaders)
            {
                callback(ptr, header);
            }
        }

        // Get all tracked object pointers (for root scanning)
        std::vector<void*> getAllTrackedPointers() const;

        // Cleanup dead objects (weak_ptr expired)
        size_t cleanupDeadObjects();

    private:
        GCTracker() = default;
        ~GCTracker() = default;
        GCTracker(const GCTracker&) = delete;
        GCTracker& operator=(const GCTracker&) = delete;
    };
}
