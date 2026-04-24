#include "GC.hpp"

namespace gc
{
    // Static member initialization
    std::unique_ptr<GCCoordinator> GC::coordinator = nullptr;
    bool GC::initialized = false;

    void GC::initialize()
    {
        
        if (!initialized)
        {
            coordinator = std::make_unique<GCCoordinator>();

            // Set up the reference visitor
            coordinator->setReferenceVisitor([](void* object, std::function<void(void*)> callback) {
                GCObjectHeader* header = GCTracker::getInstance().getHeader(object);
                if (header)
                {
                    visitReferences(object, header->type, callback);
                }
            });

            initialized = true;
        }
    }

    void GC::shutdown()
    {
       
        if (initialized)
        {
            coordinator.reset();
            GCTracker::destroyInstance();
            initialized = false;
        }
    }

    GCCoordinator* GC::get()
    {
        return coordinator.get();
    }

    bool GC::isInitialized()
    {
        return initialized;
    }

    void GC::onRefCountDecrement(void* object)
    {
        if (coordinator)
        {
            coordinator->onRefCountDecrement(object);
        }
    }

    void GC::maybeCollect()
    {
        if (coordinator)
        {
            coordinator->maybeCollect();
        }
    }

    void GC::forceCollect()
    {
        if (coordinator)
        {
            coordinator->forceCollect();
        }
    }

    void GC::printStats()
    {
        if (coordinator)
        {
            coordinator->getStats().print();
        }
        else
        {
            std::cout << "GC not initialized\n";
        }
    }

    const GCStats* GC::getStats()
    {
        if (coordinator)
        {
            return &coordinator->getStats();
        }
        return nullptr;
    }

    void GC::reset()
    {
        if (coordinator)
        {
            coordinator->reset();
        }
        else
        {
            // Even if coordinator isn't initialized, reset tracker directly
            GCTracker::getInstance().reset();
        }
    }
}
