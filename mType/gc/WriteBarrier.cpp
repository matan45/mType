#include "WriteBarrier.hpp"
#include <cstddef>
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../vm/runtime/context/ExecutionContext.hpp"
#include "../value/NativeArray.hpp"
#include "../value/FlatMultiArray.hpp"
#include "../value/SparseMultiArray.hpp"
#include "../value/PromiseValue.hpp"

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

        // Only visit if array can contain object references
        if (array->getElementType() != value::ValueType::OBJECT &&
            array->getElementType() != value::ValueType::VOID)
        {
            return;
        }

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

        size_t sz = array->totalSize();
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

    void visitSparseMultiArrayReferences(void* object, std::function<void(void*)> callback)
    {
        if (!object) return;

        auto* array = static_cast<value::SparseMultiArray*>(object);

        size_t sz = array->totalSize();
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

    void visitPromiseReferences(void* object, std::function<void(void*)> callback)
    {
        if (!object) return;

        auto* promise = static_cast<value::PromiseValue*>(object);

        // Visit resolved value if fulfilled
        if (promise->isFulfilled())
        {
            value::Value val = promise->getValue();
            void* ptr = extractPointer(val);
            if (ptr)
            {
                callback(ptr);
            }
        }

        // Visit exception value if rejected
        if (promise->isRejected())
        {
            value::Value exVal = promise->getExceptionValue();
            void* ptr = extractPointer(exVal);
            if (ptr)
            {
                callback(ptr);
            }
        }
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
                if (lambda->capturedFrame)
                {
                    lambda->capturedFrame->locals.clear();
                    lambda->capturedFrame->parentFrame.reset();
                }
                break;
            }

            case config::GCObjectType::NATIVE_ARRAY:
            {
                auto* array = static_cast<value::NativeArray*>(object);
                if (array->getElementType() == value::ValueType::OBJECT ||
                    array->getElementType() == value::ValueType::VOID)
                {
                    size_t sz = array->size();
                    for (size_t i = 0; i < sz; ++i)
                    {
                        array->set(i, value::Value{std::monostate{}});
                    }
                }
                break;
            }

            case config::GCObjectType::FLAT_MULTI_ARRAY:
            {
                auto* array = static_cast<value::FlatMultiArray*>(object);
                size_t sz = array->totalSize();
                for (size_t i = 0; i < sz; ++i)
                {
                    array->set(i, std::monostate{});
                }
                break;
            }

            case config::GCObjectType::SPARSE_MULTI_ARRAY:
            {
                auto* array = static_cast<value::SparseMultiArray*>(object);
                array->reset();
                break;
            }

            case config::GCObjectType::PROMISE_VALUE:
            {
                auto* promise = static_cast<value::PromiseValue*>(object);
                promise->clearForGC();
                break;
            }

            default:
                break;
        }
    }
}
