#pragma once

#include <memory>
#include <variant>
#include "GCCoordinator.hpp"
#include "../value/ValueType.hpp"

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
     * @brief Extract raw pointer from a Value variant
     *
     * Returns the raw pointer if the Value contains a GC-managed type,
     * nullptr otherwise.
     */
    inline void* extractPointer(const value::Value& val)
    {
        return std::visit([](auto&& arg) -> void* {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>)
            {
                return arg.get();
            }
            else if constexpr (std::is_same_v<T, std::shared_ptr<value::NativeArray>>)
            {
                return arg.get();
            }
            else if constexpr (std::is_same_v<T, std::shared_ptr<value::FlatMultiArray>>)
            {
                return arg.get();
            }
            else if constexpr (std::is_same_v<T, std::shared_ptr<value::SparseMultiArray>>)
            {
                return arg.get();
            }
            else if constexpr (std::is_same_v<T, std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>)
            {
                return arg.get();
            }
            else if constexpr (std::is_same_v<T, std::shared_ptr<vm::runtime::BytecodeLambda>>)
            {
                return arg.get();
            }
            else if constexpr (std::is_same_v<T, std::shared_ptr<value::PromiseValue>>)
            {
                return arg.get();
            }
            else
            {
                return nullptr;
            }
        }, val);
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
     * @brief Reference visitor for NativeArray
     *
     * Visits all GC-managed references held by an array.
     */
    void visitArrayReferences(void* object, std::function<void(void*)> callback);

    /**
     * @brief Combined reference visitor
     *
     * Determines object type and delegates to appropriate visitor.
     * This is the main entry point for GC reference traversal.
     */
    void visitReferences(void* object, config::GCObjectType type, std::function<void(void*)> callback);
}
