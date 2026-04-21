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

    runtimeTypes::klass::ObjectInstance*
    ObjectInstancePool::popOrNull(runtimeTypes::klass::ClassDefinition* classDef)
    {
        ++globalStats.totalAllocations;

        auto& bucket = pools[classDef];
        ++bucket.stats.totalAllocations;

        if (!bucket.slots.empty())
        {
            auto* obj = bucket.slots.back();
            bucket.slots.pop_back();
            ++bucket.stats.poolHits;
            ++globalStats.poolHits;
            bucket.stats.currentPoolSize = bucket.slots.size();
            return obj;
        }

        ++bucket.stats.poolMisses;
        ++globalStats.poolMisses;
        return nullptr;
    }

    void ObjectInstancePool::pushOrDestroy(
        runtimeTypes::klass::ClassDefinition* classDef,
        runtimeTypes::klass::ObjectInstance* obj)
    {
        auto& bucket = pools[classDef];

        if (bucket.slots.size() < bucket.maxSize)
        {
            bucket.slots.push_back(obj);
            ++bucket.stats.poolReturns;
            ++globalStats.poolReturns;
            bucket.stats.currentPoolSize = bucket.slots.size();
            if (bucket.stats.currentPoolSize > bucket.stats.maxPoolSize)
            {
                bucket.stats.maxPoolSize = bucket.stats.currentPoolSize;
            }
            return;
        }

        ++bucket.stats.poolDiscards;
        ++globalStats.poolDiscards;
        // Bucket is full: tear down completely.
        obj->~ObjectInstance();
        ::operator delete(obj);
    }

    void ObjectInstancePool::SlotDeleter::operator()(runtimeTypes::klass::ObjectInstance* p) const
    {
        // Recycle path: drop per-instance state but keep the unordered_map
        // bucket arrays so the next acquire on this slot avoids re-allocating
        // them. Field destruction inside resetForRecycle releases child Value
        // bridges and shared_ptr<MethodDefinition> entries; classDefinition
        // is also released. GCTracker holds weak_ptrs that lazily expire on
        // the next cycle-detector pass.
        p->resetForRecycle();
        pool->pushOrDestroy(classDef, p);
    }

    std::shared_ptr<runtimeTypes::klass::ObjectInstance> ObjectInstancePool::acquire(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef)
    {
        auto* classKey = classDef.get();
        auto* obj = popOrNull(classKey);
        if (obj)
        {
            obj->reinitForRecycle(std::move(classDef), {});
        }
        else
        {
            void* slot = ::operator new(sizeof(runtimeTypes::klass::ObjectInstance));
            obj = ::new (slot) runtimeTypes::klass::ObjectInstance(std::move(classDef));
        }
        return std::shared_ptr<runtimeTypes::klass::ObjectInstance>(
            obj, SlotDeleter{this, classKey});
    }

    std::shared_ptr<runtimeTypes::klass::ObjectInstance> ObjectInstancePool::acquire(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
        const std::unordered_map<std::string, std::string>& genericBindings)
    {
        auto* classKey = classDef.get();
        auto* obj = popOrNull(classKey);
        if (obj)
        {
            obj->reinitForRecycle(std::move(classDef), genericBindings);
        }
        else
        {
            void* slot = ::operator new(sizeof(runtimeTypes::klass::ObjectInstance));
            obj = ::new (slot) runtimeTypes::klass::ObjectInstance(std::move(classDef), genericBindings);
        }
        return std::shared_ptr<runtimeTypes::klass::ObjectInstance>(
            obj, SlotDeleter{this, classKey});
    }

    runtimeTypes::klass::ObjectInstance* ObjectInstancePool::acquireRaw(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
        const std::unordered_map<std::string, std::string>& genericBindings)
    {
        auto* classKey = classDef.get();
        auto* obj = popOrNull(classKey);
        if (obj)
        {
            obj->reinitForRecycle(std::move(classDef), genericBindings);
        }
        else
        {
            void* slot = ::operator new(sizeof(runtimeTypes::klass::ObjectInstance));
            obj = ::new (slot) runtimeTypes::klass::ObjectInstance(std::move(classDef), genericBindings);
        }
        return obj;
    }

    void ObjectInstancePool::releaseRaw(runtimeTypes::klass::ObjectInstance* obj)
    {
        if (!obj) return;
        // Snapshot classKey before resetForRecycle nulls it out.
        auto* classKey = const_cast<runtimeTypes::klass::ClassDefinition*>(
            obj->getClassDefinitionRaw());
        obj->resetForRecycle();
        pushOrDestroy(classKey, obj);
    }

    ObjectPoolStats ObjectInstancePool::getGlobalStats() const
    {
        return globalStats;
    }

    void ObjectInstancePool::clear()
    {
        for (auto& kv : pools)
        {
            for (auto* obj : kv.second.slots)
            {
                obj->~ObjectInstance();
                ::operator delete(obj);
            }
        }
        pools.clear();
        globalStats = ObjectPoolStats{};
    }
}
