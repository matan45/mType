#include "ObjectInstancePool.hpp"

#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"

#include <new>
#include <utility>

namespace value
{
    ObjectInstancePool& ObjectInstancePool::getInstance()
    {
        // Intentional leak: the pool must outlive every shared_ptr<ObjectInstance>
        // whose deleter holds a back-pointer here. Static-local destruction order
        // across translation units is otherwise unsafe.
        static ObjectInstancePool* instance = new ObjectInstancePool();
        return *instance;
    }

    void* ObjectInstancePool::popSlotOrAllocate(runtimeTypes::klass::ClassDefinition* classDef)
    {
        std::lock_guard<std::mutex> lock(poolMutex);
        ++globalStats.totalAllocations;

        auto& bucket = pools[classDef];
        ++bucket.stats.totalAllocations;

        if (!bucket.slots.empty()) {
            void* slot = bucket.slots.back();
            bucket.slots.pop_back();
            ++bucket.stats.poolHits;
            ++globalStats.poolHits;
            bucket.stats.currentPoolSize = bucket.slots.size();
            return slot;
        }

        ++bucket.stats.poolMisses;
        ++globalStats.poolMisses;
        return ::operator new(sizeof(runtimeTypes::klass::ObjectInstance));
    }

    void ObjectInstancePool::pushSlotOrDelete(runtimeTypes::klass::ClassDefinition* classDef, void* slot)
    {
        std::lock_guard<std::mutex> lock(poolMutex);
        auto& bucket = pools[classDef];

        if (bucket.slots.size() < bucket.maxSize) {
            bucket.slots.push_back(slot);
            ++bucket.stats.poolReturns;
            ++globalStats.poolReturns;
            bucket.stats.currentPoolSize = bucket.slots.size();
            if (bucket.stats.currentPoolSize > bucket.stats.maxPoolSize) {
                bucket.stats.maxPoolSize = bucket.stats.currentPoolSize;
            }
        }
        else {
            ++bucket.stats.poolDiscards;
            ++globalStats.poolDiscards;
            ::operator delete(slot);
        }
    }

    void ObjectInstancePool::SlotDeleter::operator()(runtimeTypes::klass::ObjectInstance* p) const
    {
        p->~ObjectInstance();
        pool->pushSlotOrDelete(classDef, static_cast<void*>(p));
    }

    std::shared_ptr<runtimeTypes::klass::ObjectInstance> ObjectInstancePool::acquire(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef)
    {
        auto* classKey = classDef.get();
        void* slot = popSlotOrAllocate(classKey);
        auto* obj = ::new (slot) runtimeTypes::klass::ObjectInstance(std::move(classDef));
        return std::shared_ptr<runtimeTypes::klass::ObjectInstance>(
            obj, SlotDeleter{this, classKey});
    }

    std::shared_ptr<runtimeTypes::klass::ObjectInstance> ObjectInstancePool::acquire(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
        const std::unordered_map<std::string, std::string>& genericBindings)
    {
        auto* classKey = classDef.get();
        void* slot = popSlotOrAllocate(classKey);
        auto* obj = ::new (slot) runtimeTypes::klass::ObjectInstance(std::move(classDef), genericBindings);
        return std::shared_ptr<runtimeTypes::klass::ObjectInstance>(
            obj, SlotDeleter{this, classKey});
    }

    ObjectPoolStats ObjectInstancePool::getGlobalStats() const
    {
        std::lock_guard<std::mutex> lock(poolMutex);
        return globalStats;
    }

    void ObjectInstancePool::clear()
    {
        std::lock_guard<std::mutex> lock(poolMutex);
        for (auto& kv : pools) {
            for (void* slot : kv.second.slots) {
                ::operator delete(slot);
            }
        }
        pools.clear();
        globalStats = ObjectPoolStats{};
    }
}
