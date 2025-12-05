#include "GCTracker.hpp"

namespace gc
{
    // Static member initialization
    GCTracker* GCTracker::instance = nullptr;
    std::mutex GCTracker::instanceMutex;

    GCTracker& GCTracker::getInstance()
    {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (instance == nullptr)
        {
            instance = new GCTracker();
        }
        return *instance;
    }

    void GCTracker::destroyInstance()
    {
        std::lock_guard<std::mutex> lock(instanceMutex);
        delete instance;
        instance = nullptr;
    }

    void GCTracker::unregisterObject(void* rawPtr)
    {
        if (!rawPtr) return;

        std::lock_guard<std::mutex> lock(trackerMutex);
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
        std::lock_guard<std::mutex> lock(trackerMutex);
        auto it = objectHeaders.find(rawPtr);
        if (it != objectHeaders.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    const GCObjectHeader* GCTracker::getHeader(void* rawPtr) const
    {
        std::lock_guard<std::mutex> lock(trackerMutex);
        auto it = objectHeaders.find(rawPtr);
        if (it != objectHeaders.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    bool GCTracker::isObjectAlive(void* rawPtr) const
    {
        std::lock_guard<std::mutex> lock(trackerMutex);
        auto it = weakReferences.find(rawPtr);
        if (it != weakReferences.end())
        {
            return !it->second.expired();
        }
        return false;
    }

    std::vector<void*> GCTracker::getAllTrackedPointers() const
    {
        std::lock_guard<std::mutex> lock(trackerMutex);
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
        std::lock_guard<std::mutex> lock(trackerMutex);
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
}
