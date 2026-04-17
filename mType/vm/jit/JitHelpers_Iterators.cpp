#include "JitHelpers.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../../environment/Environment.hpp"
#include "../../environment/registry/ClassRegistry.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/ValueObject.hpp"
#include "../runtime/VirtualMachine.hpp"
#include <memory>
#include <vector>

// MYT-147: Shape-A runtime helpers for the iterator protocol. Each helper is
// invoked from JIT-compiled code; the JIT emitter marshals the receiver into
// ctx->callArgs[0] before calling. Helpers mirror the interpreter behavior in
// ObjectExecutor::handleGetIterator / handleIteratorHasNext / handleIteratorNext /
// handleIteratorClose exactly.

namespace vm::jit
{
    namespace
    {
        // Wrap a raw array receiver in an ArrayIteratorHelper instance.
        // Mirrors ObjectExecutor::handleGetIterator at ObjectExecutor.cpp:927-955.
        value::Value buildArrayIteratorHelper(vm::runtime::VirtualMachine* vm,
                                              const value::Value& arrayVal)
        {
            if (!vm)
                throw errors::RuntimeException("JIT iterator: null VM");
            auto env = vm->getEnvironment();
            if (!env)
                throw errors::RuntimeException("JIT iterator: null environment");
            auto classRegistry = env->getClassRegistry();
            if (!classRegistry)
                throw errors::RuntimeException("JIT iterator: null class registry");
            auto iteratorHelperClass = classRegistry->findClass("ArrayIteratorHelper");
            if (!iteratorHelperClass)
                throw errors::RuntimeException(
                    "ArrayIteratorHelper class not found - required for array iteration");
            auto iteratorInstance =
                std::make_shared<runtimeTypes::klass::ObjectInstance>(iteratorHelperClass);
            iteratorInstance->setField("array", arrayVal);
            iteratorInstance->setField("index", static_cast<int64_t>(0));
            return value::Value(iteratorInstance);
        }

        std::shared_ptr<runtimeTypes::klass::ObjectInstance>
        asInstance(const value::Value& v)
        {
            if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(v))
                return std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(v);
            return nullptr;
        }
    }

    // GET_ITERATOR: pop collection from callArgs[0], push iterator via *dest.
    void jit_iterator_get(value::Value* dest, JitContext* ctx)
    {
        if (ctx->pendingException) return;
        try
        {
            value::Value& receiver = ctx->callArgs[0];

            if (std::holds_alternative<std::nullptr_t>(receiver) ||
                std::holds_alternative<std::monostate>(receiver))
            {
                throw errors::NullPointerException(
                    "Cannot get iterator from null collection");
            }

            // Array fast path: wrap in ArrayIteratorHelper directly.
            if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(receiver))
            {
                *dest = buildArrayIteratorHelper(ctx->vm, receiver);
                return;
            }

            auto collection = asInstance(receiver);
            if (!collection)
                throw errors::RuntimeException(
                    "GET_ITERATOR requires an object instance");
            if (!ctx->vm)
                throw errors::RuntimeException("JIT iterator: null VM");

            *dest = ctx->vm->callMethodFromJit(collection, "iterator", {});
        }
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
    }

    // ITERATOR_HAS_NEXT: peek receiver in callArgs[0] (caller does NOT decrement
    // stackDepth for the receiver), push bool via *dest.
    void jit_iterator_has_next(value::Value* dest, JitContext* ctx)
    {
        if (ctx->pendingException) return;
        try
        {
            value::Value& receiver = ctx->callArgs[0];

            if (std::holds_alternative<std::nullptr_t>(receiver) ||
                std::holds_alternative<std::monostate>(receiver))
            {
                throw errors::NullPointerException(
                    "Cannot call hasNext() on null iterator");
            }

            auto iterator = asInstance(receiver);
            if (!iterator)
                throw errors::RuntimeException(
                    "ITERATOR_HAS_NEXT requires an iterator instance");
            if (!ctx->vm)
                throw errors::RuntimeException("JIT iterator: null VM");

            *dest = ctx->vm->callMethodFromJit(iterator, "hasNext", {});
        }
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
    }

    // ITERATOR_NEXT: peek receiver in callArgs[0], push next element via *dest.
    void jit_iterator_next(value::Value* dest, JitContext* ctx)
    {
        if (ctx->pendingException) return;
        try
        {
            value::Value& receiver = ctx->callArgs[0];

            if (std::holds_alternative<std::nullptr_t>(receiver) ||
                std::holds_alternative<std::monostate>(receiver))
            {
                throw errors::NullPointerException(
                    "Cannot call next() on null iterator");
            }

            auto iterator = asInstance(receiver);
            if (!iterator)
                throw errors::RuntimeException(
                    "ITERATOR_NEXT requires an iterator instance");
            if (!ctx->vm)
                throw errors::RuntimeException("JIT iterator: null VM");

            *dest = ctx->vm->callMethodFromJit(iterator, "next", {});
        }
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
    }

    // ITERATOR_CLOSE: pop receiver from callArgs[0], call close(), swallow all
    // exceptions (matches interpreter's try-with-resources behavior at
    // ObjectExecutor.cpp:1040-1048). No dest - close() return is discarded.
    void jit_iterator_close(JitContext* ctx)
    {
        if (ctx->pendingException) return;
        value::Value& receiver = ctx->callArgs[0];

        if (std::holds_alternative<std::nullptr_t>(receiver) ||
            std::holds_alternative<std::monostate>(receiver))
        {
            return;  // null is fine - nothing to close
        }

        auto iterator = asInstance(receiver);
        if (!iterator) return;  // non-object receiver - just ignore (matches interpreter)
        if (!ctx->vm) return;

        try
        {
            (void)ctx->vm->callMethodFromJit(iterator, "close", {});
        }
        catch (...)
        {
            // Swallow: close() is best-effort cleanup.
        }
    }
}
