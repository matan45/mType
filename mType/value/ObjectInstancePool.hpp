#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace runtimeTypes::klass
{
    class ObjectInstance;
    class ClassDefinition;
}

namespace value
{
    struct ObjectPoolStats
    {
        size_t totalAllocations = 0;
        size_t poolHits = 0;
        size_t poolMisses = 0;
        size_t poolReturns = 0;
        size_t poolDiscards = 0;
        size_t currentPoolSize = 0;
        size_t maxPoolSize = 0;

        double getHitRate() const
        {
            return totalAllocations > 0
                ? static_cast<double>(poolHits) / static_cast<double>(totalAllocations)
                : 0.0;
        }
    };

    /**
     * Per-class freelist allocator for ObjectInstance (MYT-171).
     *
     * Threading: mType is single-threaded today (interpreter loop and JIT-emitted
     * code share one thread). The internal mutex is uncontended in this regime
     * and is retained for forward compatibility with future worker-thread support.
     * When multi-threading lands, consider sharding buckets by thread-local cache
     * with a global overflow tier.
     *
     * Lifecycle: the pool stores live ObjectInstance objects (not raw memory)
     * in per-class freelists. acquire() pops one and calls reinitForRecycle to
     * re-bind it to the requested classDefinition and generic bindings; on a
     * miss it ::operator new + placement-news a fresh ObjectInstance. Either
     * way the result is wrapped in a shared_ptr with a custom deleter that
     * runs resetForRecycle (clears field state, releases child references,
     * keeps the unordered_map bucket arrays) and pushes the object back. The
     * second-to-Nth acquire on the same per-class slot therefore avoids the
     * unordered_map bucket-array allocation entirely.
     *
     * Discarded slots (when a bucket is at cap) are torn down with
     * ~ObjectInstance + ::operator delete.
     *
     * GC interaction: GCTracker holds weak_ptrs that expire when strong_count
     * hits zero (deleter runs); they are lazily swept by the next cycle
     * detector pass. shared_from_this is correctly re-armed by each new
     * shared_ptr we return — the enable_shared_from_this base's _M_weak_this
     * is overwritten by the shared_ptr constructor.
     *
     * The singleton is intentionally leaked at process exit so the deleter
     * never dereferences a destroyed pool when long-lived shared_ptrs
     * (e.g. cached boxed primitives) outlive other static-local objects.
     */
    class ObjectInstancePool
    {
    public:
        static ObjectInstancePool& getInstance();

        std::shared_ptr<runtimeTypes::klass::ObjectInstance> acquire(
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef);

        std::shared_ptr<runtimeTypes::klass::ObjectInstance> acquire(
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
            const std::unordered_map<std::string, std::string>& genericBindings);

        ObjectPoolStats getGlobalStats() const;
        void clear();

        static constexpr size_t DEFAULT_BUCKET_CAP = 32;

    private:
        struct Bucket
        {
            std::vector<runtimeTypes::klass::ObjectInstance*> slots;
            ObjectPoolStats stats;
            size_t maxSize = DEFAULT_BUCKET_CAP;
        };

        struct SlotDeleter
        {
            ObjectInstancePool* pool;
            runtimeTypes::klass::ClassDefinition* classDef;
            void operator()(runtimeTypes::klass::ObjectInstance* p) const;
        };

        ObjectInstancePool() = default;
        ObjectInstancePool(const ObjectInstancePool&) = delete;
        ObjectInstancePool& operator=(const ObjectInstancePool&) = delete;

        runtimeTypes::klass::ObjectInstance* popOrNull(
            runtimeTypes::klass::ClassDefinition* classDef);
        void pushOrDestroy(runtimeTypes::klass::ClassDefinition* classDef,
                           runtimeTypes::klass::ObjectInstance* obj);

        std::unordered_map<runtimeTypes::klass::ClassDefinition*, Bucket> pools;
        mutable std::mutex poolMutex;
        ObjectPoolStats globalStats;
    };
}
