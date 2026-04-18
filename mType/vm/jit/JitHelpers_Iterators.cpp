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

        // MYT-155: monomorphic-shape fast paths for the two iterator classes that
        // ship with the standard library. ArrayIteratorHelper backs raw-array
        // for-each (handleGetIterator constructs it), ListIterator backs ArrayList.
        // Both are pure index-bump + bounds-check, so we can answer hasNext() /
        // next() without re-entering the VM. Returns true if the fast path
        // applied (and wrote *dest); false otherwise — caller falls back to the
        // generic vm->callMethodFromJit path.

        bool tryFastHasNext(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& iter,
                             value::Value* dest)
        {
            const std::string& className = iter->getClassDefinition()->getName();
            if (className == "ArrayIteratorHelper")
            {
                const value::Value& idxVal = iter->getFieldValue("index");
                const value::Value& arrVal = iter->getFieldValue("array");
                if (!std::holds_alternative<int64_t>(idxVal)) return false;
                if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(arrVal)) return false;
                auto arr = std::get<std::shared_ptr<value::NativeArray>>(arrVal);
                if (!arr) return false;
                *dest = (std::get<int64_t>(idxVal) < static_cast<int64_t>(arr->size()));
                return true;
            }
            if (className == "ListIterator")
            {
                const value::Value& idxVal = iter->getFieldValue("currentIndex");
                const value::Value& sizeVal = iter->getFieldValue("listSize");
                if (!std::holds_alternative<int64_t>(idxVal)) return false;
                if (!std::holds_alternative<int64_t>(sizeVal)) return false;
                *dest = (std::get<int64_t>(idxVal) < std::get<int64_t>(sizeVal));
                return true;
            }
            return false;
        }

        bool tryFastNext(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& iter,
                          value::Value* dest)
        {
            const std::string& className = iter->getClassDefinition()->getName();
            if (className == "ArrayIteratorHelper")
            {
                const value::Value& idxVal = iter->getFieldValue("index");
                const value::Value& arrVal = iter->getFieldValue("array");
                if (!std::holds_alternative<int64_t>(idxVal)) return false;
                if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(arrVal)) return false;
                auto arr = std::get<std::shared_ptr<value::NativeArray>>(arrVal);
                if (!arr) return false;
                int64_t idx = std::get<int64_t>(idxVal);
                if (idx < 0 || static_cast<size_t>(idx) >= arr->size()) return false;
                *dest = arr->getUnchecked(static_cast<size_t>(idx));
                iter->setField("index", static_cast<int64_t>(idx + 1));
                return true;
            }
            if (className == "ListIterator")
            {
                const value::Value& idxVal = iter->getFieldValue("currentIndex");
                const value::Value& sizeVal = iter->getFieldValue("listSize");
                const value::Value& dataVal = iter->getFieldValue("data");
                if (!std::holds_alternative<int64_t>(idxVal)) return false;
                if (!std::holds_alternative<int64_t>(sizeVal)) return false;
                if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(dataVal)) return false;
                auto arr = std::get<std::shared_ptr<value::NativeArray>>(dataVal);
                if (!arr) return false;
                int64_t idx = std::get<int64_t>(idxVal);
                if (idx < 0 || idx >= std::get<int64_t>(sizeVal)) return false;
                if (static_cast<size_t>(idx) >= arr->size()) return false;
                *dest = arr->getUnchecked(static_cast<size_t>(idx));
                iter->setField("currentIndex", static_cast<int64_t>(idx + 1));
                return true;
            }
            return false;
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

    // ITERATOR_HAS_NEXT: receiver is in callArgs[0] (emitter copies it from the
    // operand-stack receiver slot then pops, MYT-156). Push bool via *dest.
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

            // MYT-155: monomorphic-shape fast path skips VM re-entry when the
            // receiver is one of the two stdlib iterator classes.
            if (tryFastHasNext(iterator, dest)) return;

            *dest = ctx->vm->callMethodFromJit(iterator, "hasNext", {});
        }
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
    }

    // ITERATOR_NEXT: receiver is in callArgs[0] (emitter copies and pops,
    // MYT-156). Push next element via *dest.
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

            // MYT-155: monomorphic-shape fast path skips VM re-entry.
            if (tryFastNext(iterator, dest)) return;

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
