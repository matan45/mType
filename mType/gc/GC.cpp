#include "GC.hpp"
#include "../value/ObjectInstance.hpp"

namespace gc
{
    namespace
    {
        void observeHeapValue(
            value::BridgeKind kind,
            const std::shared_ptr<void>& object)
        {
            config::GCObjectType type = config::GCObjectType::UNKNOWN;
            switch (kind)
            {
            case value::BridgeKind::OBJECT_INSTANCE:
                std::static_pointer_cast<runtimeTypes::klass::ObjectInstance>(object)
                    ->registerWithGC();
                return;
            case value::BridgeKind::BYTECODE_LAMBDA:
                type = config::GCObjectType::BYTECODE_LAMBDA;
                break;
            case value::BridgeKind::NATIVE_ARRAY:
                type = config::GCObjectType::NATIVE_ARRAY;
                break;
            case value::BridgeKind::FLAT_MULTI_ARRAY:
                type = config::GCObjectType::FLAT_MULTI_ARRAY;
                break;
            case value::BridgeKind::SPARSE_MULTI_ARRAY:
                type = config::GCObjectType::SPARSE_MULTI_ARRAY;
                break;
            case value::BridgeKind::FLAT_MULTI_OBJECT_ARRAY:
                type = config::GCObjectType::FLAT_MULTI_OBJECT_ARRAY;
                break;
            case value::BridgeKind::PROMISE:
                type = config::GCObjectType::PROMISE_VALUE;
                break;
            default:
                return;
            }
            GC::registerAllocation(object, type);
        }

        void observeReferenceMutation(
            void* owner,
            const value::Value* oldValue,
            const value::Value* newValue)
        {
            void* oldTarget = oldValue ? extractPointer(*oldValue) : nullptr;
            void* newTarget = newValue ? extractPointer(*newValue) : nullptr;
            if (oldTarget && oldTarget != newTarget)
            {
                GC::onRefCountDecrement(oldTarget);
            }
            if (owner && newTarget)
            {
                GC::onRefCountDecrement(owner);
            }
            else if (!owner && newTarget)
            {
                // SharedStackFrame has no independently tracked owner. Marking
                // the new target is conservative and still reaches a cycle
                // closed through the lambda that owns the frame.
                GC::onRefCountDecrement(newTarget);
            }
        }

        void observeRawReferenceRemoval(void* target)
        {
            GC::onRefCountDecrement(target);
        }
    }

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

            value::setHeapValueRegistrationObserver(&observeHeapValue);
            value::setHeapReferenceMutationObserver(&observeReferenceMutation);
            value::setHeapRawReferenceRemovalObserver(&observeRawReferenceRemoval);

            initialized = true;
        }
    }

    void GC::shutdown()
    {
       
        if (initialized)
        {
            value::setHeapRawReferenceRemovalObserver(nullptr);
            value::setHeapReferenceMutationObserver(nullptr);
            value::setHeapValueRegistrationObserver(nullptr);
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
