#include "WriteBarrier.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../vm/runtime/context/ExecutionContext.hpp"
#include "../value/NativeArray.hpp"

namespace gc
{
    void visitObjectInstanceReferences(void* object, std::function<void(void*)> callback)
    {
        if (!object) return;

        auto* instance = static_cast<runtimeTypes::klass::ObjectInstance*>(object);

        // Visit all field values
        for (const auto& [name, fieldValue] : instance->getAllFieldValues())
        {
            void* ptr = extractPointer(fieldValue);
            if (ptr)
            {
                callback(ptr);
            }
        }
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

    void visitArrayReferences(void* object, std::function<void(void*)> callback)
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
            case config::GCObjectType::FLAT_MULTI_ARRAY:
            case config::GCObjectType::SPARSE_MULTI_ARRAY:
                visitArrayReferences(object, callback);
                break;

            case config::GCObjectType::PROMISE_VALUE:
                // PromiseValue may hold a result value - visit it
                // TODO: Implement if PromiseValue can hold object references
                break;

            default:
                break;
        }
    }
}
