#include "WriteBarrier.hpp"
#include <cstddef>
#include "../value/ObjectInstance.hpp"
#include "../vm/runtime/context/ExecutionContext.hpp"
#include "../value/NativeArray.hpp"
#include "../value/FlatMultiArray.hpp"
#include "../value/SparseMultiArray.hpp"
#include "../value/PromiseValue.hpp"
#include "../value/arrays/object/FlatMultiObjectArray.hpp"

namespace gc
{
    void visitObjectInstanceReferences(void* object, std::function<void(void*)> callback)
    {
        if (!object) return;

        auto* instance = static_cast<runtimeTypes::klass::ObjectInstance*>(object);
        instance->visitReferences(callback);
    }

    void visitLambdaReferences(void* object, std::function<void(void*)> callback)
    {
        if (!object) return;

        auto* lambda = static_cast<vm::runtime::BytecodeLambda*>(object);

        // Visit captured 'this'
        if (lambda->capturedThis)
        {
            callback(lambda->capturedThis.get());
        }

        // Visit captured frame locals
        if (lambda->capturedFrame)
        {
            for (const auto& local : lambda->capturedFrame->locals)
            {
                void* ptr = extractPointer(local);
                if (ptr)
                {
                    callback(ptr);
                }
            }

            // Visit parent frames recursively
            auto parentFrame = lambda->capturedFrame->parentFrame;
            while (parentFrame)
            {
                for (const auto& local : parentFrame->locals)
                {
                    void* ptr = extractPointer(local);
                    if (ptr)
                    {
                        callback(ptr);
                    }
                }
                parentFrame = parentFrame->parentFrame;
            }
        }
    }

    void visitNativeArrayReferences(void* object, std::function<void(void*)> callback)
    {
        if (!object) return;

        auto* array = static_cast<value::NativeArray*>(object);

        // Exact-class SoA storage contains only primitive field columns (the
        // ObjectArray constructor rejects reference-typed fields). Calling
        // NativeArray::get() here would materialize fresh ObjectInstances and
        // invent GC edges that are not present in the stored representation.
        if (array->getObjectArrayData()) return;

        // Visit all elements
        size_t sz = array->size();
        for (size_t i = 0; i < sz; ++i)
        {
            value::Value val = array->get(i);
            void* ptr = extractPointer(val);
            if (ptr)
            {
                callback(ptr);
            }
        }
    }

    void visitFlatMultiArrayReferences(void* object, std::function<void(void*)> callback)
    {
        if (!object) return;

        auto* array = static_cast<value::FlatMultiArray*>(object);

        if (auto* parent = array->getParentForGC())
        {
            callback(parent);
        }

        array->visitValuesForGC([&callback](const value::Value& val) {
            void* ptr = extractPointer(val);
            if (ptr)
            {
                callback(ptr);
            }
        });
    }

    void visitSparseMultiArrayReferences(void* object, std::function<void(void*)> callback)
    {
        if (!object) return;

        auto* array = static_cast<value::SparseMultiArray*>(object);

        if (auto* parent = array->getParentForGC())
        {
            callback(parent);
        }

        array->visitValuesForGC([&callback](const value::Value& val) {
            void* ptr = extractPointer(val);
            if (ptr)
            {
                callback(ptr);
            }
        });
    }

    void visitPromiseReferences(void* object, std::function<void(void*)> callback)
    {
        if (!object) return;

        auto* promise = static_cast<value::PromiseValue*>(object);

        for (const auto& value : promise->getReferencesForGC())
        {
            void* ptr = extractPointer(value);
            if (ptr)
            {
                callback(ptr);
            }
        }
    }

    void visitFlatMultiObjectArrayReferences(
        void* object,
        std::function<void(void*)> callback)
    {
        if (!object) return;
        auto* array = static_cast<mType::value::arrays::FlatMultiObjectArray*>(object);
        if (auto* parent = array->getParentForGC())
        {
            callback(parent);
        }
        array->visitValuesForGC([&callback](const value::Value& value) {
            void* ptr = extractPointer(value);
            if (ptr)
            {
                callback(ptr);
            }
        });
    }

    void visitReferences(void* object, config::GCObjectType type, std::function<void(void*)> callback)
    {
        switch (type)
        {
            case config::GCObjectType::OBJECT_INSTANCE:
                visitObjectInstanceReferences(object, callback);
                break;

            case config::GCObjectType::BYTECODE_LAMBDA:
                visitLambdaReferences(object, callback);
                break;

            case config::GCObjectType::NATIVE_ARRAY:
                visitNativeArrayReferences(object, callback);
                break;

            case config::GCObjectType::FLAT_MULTI_ARRAY:
                visitFlatMultiArrayReferences(object, callback);
                break;

            case config::GCObjectType::SPARSE_MULTI_ARRAY:
                visitSparseMultiArrayReferences(object, callback);
                break;

            case config::GCObjectType::PROMISE_VALUE:
                visitPromiseReferences(object, callback);
                break;

            case config::GCObjectType::FLAT_MULTI_OBJECT_ARRAY:
                visitFlatMultiObjectArrayReferences(object, callback);
                break;

            default:
                break;
        }
    }

    void breakReferences(void* object, config::GCObjectType type)
    {
        if (!object) return;

        switch (type)
        {
            case config::GCObjectType::OBJECT_INSTANCE:
            {
                auto* instance = static_cast<runtimeTypes::klass::ObjectInstance*>(object);
                instance->clearAllFields();
                break;
            }

            case config::GCObjectType::BYTECODE_LAMBDA:
            {
                auto* lambda = static_cast<vm::runtime::BytecodeLambda*>(object);
                lambda->capturedThis.reset();
                // The frame can be shared by other, live closures. The lambda
                // owns only this shared_ptr edge, never the frame contents or
                // parent chain.
                lambda->capturedFrame.reset();
                break;
            }

            case config::GCObjectType::NATIVE_ARRAY:
            {
                auto* array = static_cast<value::NativeArray*>(object);
                array->clearReferencesForGC();
                break;
            }

            case config::GCObjectType::FLAT_MULTI_ARRAY:
            {
                auto* array = static_cast<value::FlatMultiArray*>(object);
                array->clearReferencesForGC();
                break;
            }

            case config::GCObjectType::SPARSE_MULTI_ARRAY:
            {
                auto* array = static_cast<value::SparseMultiArray*>(object);
                array->clearReferencesForGC();
                break;
            }

            case config::GCObjectType::PROMISE_VALUE:
            {
                auto* promise = static_cast<value::PromiseValue*>(object);
                promise->clearForGC();
                break;
            }

            case config::GCObjectType::FLAT_MULTI_OBJECT_ARRAY:
            {
                auto* array = static_cast<mType::value::arrays::FlatMultiObjectArray*>(object);
                array->clearReferencesForGC();
                break;
            }

            default:
                break;
        }
    }
}
