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
     * Lifecycle: acquire() pops a raw aligned slot (or allocates one), placement-news
     * an ObjectInstance into it, and returns a shared_ptr with a custom deleter.
     * The deleter explicitly invokes ~ObjectInstance() (releasing field shared_ptrs,
     * classDefinition, methodCache, fieldVector) before returning the slot to the
     * per-class freelist. GCTracker holds weak_ptrs that lazily expire — no extra
     * coordination required.
     *
     * Invariant: placement-new MUST run on every acquire. It reinitializes the
     * enable_shared_from_this base subobject (weak_this); skipping it would be UB.
     *
     * The singleton is intentionally leaked at process exit so the deleter never
     * dereferences a destroyed pool when long-lived shared_ptrs (e.g. cached
     * boxed primitives) outlive other static-local objects.
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
            std::vector<void*> slots;
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

        void* popSlotOrAllocate(runtimeTypes::klass::ClassDefinition* classDef);
        void pushSlotOrDelete(runtimeTypes::klass::ClassDefinition* classDef, void* slot);

        std::unordered_map<runtimeTypes::klass::ClassDefinition*, Bucket> pools;
        mutable std::mutex poolMutex;
        ObjectPoolStats globalStats;
    };
}
