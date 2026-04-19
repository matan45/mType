#pragma once

#include <memory>
#include "GCCoordinator.hpp"
#include "../value/ValueShim.hpp"

namespace gc
{
    /**
     * @brief Extract raw pointer from a Value variant
     *
     * Returns the raw pointer if the Value contains a GC-managed type,
     * nullptr otherwise.
     */
    inline void* extractPointer(const value::Value& val)
    {
        if (value::isObject(val))              return value::asObject(val).get();
        if (value::isNativeArray(val))         return value::asNativeArray(val).get();
        if (value::isFlatMultiArray(val))      return value::asFlatMultiArray(val).get();
        if (value::isSparseMultiArray(val))    return value::asSparseMultiArray(val).get();
        if (value::isFlatMultiObjectArray(val)) return value::asFlatMultiObjectArray(val).get();
        if (value::isLambda(val))              return value::asLambda(val).get();
        if (value::isPromise(val))             return value::asPromise(val).get();
        return nullptr;
    }

    /**
     * @brief Write barrier for GC
     *
     * Call this when a reference field is being overwritten.
     * Notifies the GC that the old reference may become garbage.
     *
     * @param coordinator The GC coordinator
     * @param oldRef The old value being replaced (raw pointer)
     * @param newRef The new value being assigned (raw pointer)
     */
    inline void writeBarrier(GCCoordinator& coordinator, void* oldRef, void* newRef)
    {
        // If we're overwriting a reference, the old target's refcount will decrease
        if (oldRef != nullptr && oldRef != newRef)
        {
            coordinator.onRefCountDecrement(oldRef);
        }
    }

    /**
     * @brief Write barrier for Value types
     *
     * Convenience overload that extracts pointers from Value variants.
     */
    inline void writeBarrier(GCCoordinator& coordinator, const value::Value& oldVal, const value::Value& newVal)
    {
        void* oldPtr = extractPointer(oldVal);
        void* newPtr = extractPointer(newVal);
        writeBarrier(coordinator, oldPtr, newPtr);
    }

    /**
     * @brief Reference visitor for ObjectInstance
     *
     * Visits all GC-managed references held by an ObjectInstance.
     */
    void visitObjectInstanceReferences(void* object, std::function<void(void*)> callback);

    /**
     * @brief Reference visitor for BytecodeLambda
     *
     * Visits all GC-managed references captured by a lambda.
     */
    void visitLambdaReferences(void* object, std::function<void(void*)> callback);

    /**
     * @brief Reference visitor for NativeArray (1D arrays)
     *
     * Visits all GC-managed references held by a NativeArray.
     */
    void visitNativeArrayReferences(void* object, std::function<void(void*)> callback);

    /**
     * @brief Reference visitor for FlatMultiArray (multi-dimensional)
     *
     * Visits all GC-managed references held by a FlatMultiArray.
     */
    void visitFlatMultiArrayReferences(void* object, std::function<void(void*)> callback);

    /**
     * @brief Reference visitor for SparseMultiArray (multi-dimensional sparse)
     *
     * Visits all GC-managed references held by a SparseMultiArray.
     */
    void visitSparseMultiArrayReferences(void* object, std::function<void(void*)> callback);

    /**
     * @brief Reference visitor for PromiseValue
     *
     * Visits all GC-managed references held by a PromiseValue.
     */
    void visitPromiseReferences(void* object, std::function<void(void*)> callback);

    /**
     * @brief Combined reference visitor
     *
     * Determines object type and delegates to appropriate visitor.
     * This is the main entry point for GC reference traversal.
     */
    void visitReferences(void* object, config::GCObjectType type, std::function<void(void*)> callback);

    /**
     * @brief Break all outgoing references from an object
     *
     * Clears all reference fields in the object to break cycles.
     * This allows the shared_ptr refcounts to reach zero.
     * Called by the GC before unregistering cyclic garbage.
     */
    void breakReferences(void* object, config::GCObjectType type);
}
