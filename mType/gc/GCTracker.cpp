#include "GCTracker.hpp"
#include <cstddef>

namespace gc
{
    GCTracker& GCTracker::getInstance()
    {
        static GCTracker instance;
        return instance;
    }

    void GCTracker::destroyInstance()
    {
        // Meyer's Singleton lifetime is managed by the runtime, but we clear
        // all tracked state so GC::shutdown() leaves a clean slate.
        getInstance().reset();
    }

    void GCTracker::unregisterObject(void* rawPtr)
    {
        if (!rawPtr) return;
        
        auto it = objectHeaders.find(rawPtr);
        if (it != objectHeaders.end())
        {
            objectHeaders.erase(it);
            weakReferences.erase(rawPtr);
            if (totalTrackedObjects > 0)
            {
                totalTrackedObjects--;
            }
        }
    }

    GCObjectHeader* GCTracker::getHeader(void* rawPtr)
    {
        auto it = objectHeaders.find(rawPtr);
        if (it != objectHeaders.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    const GCObjectHeader* GCTracker::getHeader(void* rawPtr) const
    {
        auto it = objectHeaders.find(rawPtr);
        if (it != objectHeaders.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    bool GCTracker::isObjectAlive(void* rawPtr) const
    {
        auto it = weakReferences.find(rawPtr);
        if (it != weakReferences.end())
        {
            return !it->second.expired();
        }
        return false;
    }

    std::vector<void*> GCTracker::getAllTrackedPointers() const
    {
        std::vector<void*> pointers;
        pointers.reserve(objectHeaders.size());
        for (const auto& [ptr, _] : objectHeaders)
        {
            pointers.push_back(ptr);
        }
        return pointers;
    }

    size_t GCTracker::cleanupDeadObjects()
    {
        size_t removed = 0;

        std::vector<void*> toRemove;
        for (const auto& [ptr, weakRef] : weakReferences)
        {
            if (weakRef.expired())
            {
                toRemove.push_back(ptr);
            }
        }

        for (void* ptr : toRemove)
        {
            objectHeaders.erase(ptr);
            weakReferences.erase(ptr);
            removed++;
        }

        if (totalTrackedObjects >= removed)
        {
            totalTrackedObjects -= removed;
        }
        else
        {
            totalTrackedObjects = 0;
        }

        return removed;
    }

    void GCTracker::reset()
    {
        objectHeaders.clear();
        weakReferences.clear();
        allocationCount = 0;
        totalTrackedObjects = 0;
    }
}
