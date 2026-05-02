#include "JitHelpers.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include "../../value/ValueShim.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../../environment/Environment.hpp"
#include "../../environment/registry/ClassRegistry.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/ObjectInstancePool.hpp"
#include "../../value/ValueObject.hpp"
#include "../runtime/VirtualMachine.hpp"
#include <memory>
#include <string>
#include <unordered_map>
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
                value::ObjectInstancePool::getInstance().acquire(iteratorHelperClass);
            iteratorInstance->setField("array", arrayVal);
            iteratorInstance->setField("index", static_cast<int64_t>(0));
            return value::Value(iteratorInstance);
        }

        std::shared_ptr<runtimeTypes::klass::ObjectInstance>
        asInstance(const value::Value& v)
        {
            if (value::isObject(v))
                return value::asObject(v);
            return nullptr;
        }

        // MYT-132 Phase B: per-class field-index cache for the two stdlib
        // iterator shapes. The previous MYT-155 fast path did two
        // unordered_map lookups (getFieldValue(string)) and a Value copy per
        // field per iteration. For a 2000*1000-iteration for-each loop that's
        // 8M hash-string operations. The cache lets us switch to indexed
        // access via getFieldByIndex (const-ref, no copy, direct vector
        // subscript). Indices are class-invariant so one-time resolution per
        // ClassDefinition is safe.
        enum class IteratorShape { UNKNOWN, ARRAY_ITERATOR, LIST_ITERATOR };

        struct IteratorFastSlots
        {
            IteratorShape shape = IteratorShape::UNKNOWN;
            size_t idxSlot = SIZE_MAX;
            size_t collSlot = SIZE_MAX;  // "array" for ArrayIteratorHelper, "data" for ListIterator
            size_t sizeSlot = SIZE_MAX;  // ListIterator only
            // Cached class name for setField path (still routes by name for
            // GC write-barrier + fieldValues map sync; cheaper than a reverse
            // index→name scan in setFieldByIndex).
            const std::string* idxFieldName = nullptr;
        };

        // thread_local so we don't need a mutex. ClassDefinition* is
        // process-lifetime stable — no invalidation concern here. If a class
        // is redefined (future MYT), Phase A's invalidateAll() hook should
        // clear this too; until then class redefinition is out of scope.
        thread_local std::unordered_map<const runtimeTypes::klass::ClassDefinition*,
                                         IteratorFastSlots> g_iteratorSlotCache;

        // Field name constants — reused by setField to avoid string literal
        // temporaries on the hot path.
        const std::string kFieldArray         = "array";
        const std::string kFieldIndex         = "index";
        const std::string kFieldData          = "data";
        const std::string kFieldCurrentIndex  = "currentIndex";
        const std::string kFieldListSize      = "listSize";

        const IteratorFastSlots* resolveSlots(
            const runtimeTypes::klass::ClassDefinition* classDef)
        {
            auto it = g_iteratorSlotCache.find(classDef);
            if (it != g_iteratorSlotCache.end())
            {
                return it->second.shape == IteratorShape::UNKNOWN
                    ? nullptr : &it->second;
            }

            IteratorFastSlots slots;
            const std::string& name = classDef->getName();
            if (name == "ArrayIteratorHelper")
            {
                slots.shape = IteratorShape::ARRAY_ITERATOR;
                slots.idxSlot = classDef->getFieldIndex(kFieldIndex);
                slots.collSlot = classDef->getFieldIndex(kFieldArray);
                slots.idxFieldName = &kFieldIndex;
                if (slots.idxSlot == SIZE_MAX || slots.collSlot == SIZE_MAX)
                    slots.shape = IteratorShape::UNKNOWN;
            }
            else if (name == "ListIterator")
            {
                slots.shape = IteratorShape::LIST_ITERATOR;
                slots.idxSlot = classDef->getFieldIndex(kFieldCurrentIndex);
                slots.collSlot = classDef->getFieldIndex(kFieldData);
                slots.sizeSlot = classDef->getFieldIndex(kFieldListSize);
                slots.idxFieldName = &kFieldCurrentIndex;
                if (slots.idxSlot == SIZE_MAX || slots.collSlot == SIZE_MAX
                    || slots.sizeSlot == SIZE_MAX)
                    slots.shape = IteratorShape::UNKNOWN;
            }

            auto [insIt, _] = g_iteratorSlotCache.emplace(classDef, slots);
            return insIt->second.shape == IteratorShape::UNKNOWN
                ? nullptr : &insIt->second;
        }

        // MYT-155 / MYT-132 Phase B: monomorphic-shape fast paths for the two
        // iterator classes that ship with the standard library. Cached
        // per-class field indices let us skip the unordered_map field lookups
        // on the iteration hot path. Returns true if the fast path applied
        // (and wrote *dest); false otherwise — caller falls back to the
        // generic vm->callMethodFromJit path.

        bool tryFastHasNext(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& iter,
                             value::Value* dest)
        {
            auto* classDef = iter->getClassDefinition().get();
            const IteratorFastSlots* slots = resolveSlots(classDef);
            if (!slots) return false;

            if (!iter->hasFieldVector())
                iter->ensureFieldVector();

            const value::Value& idxVal = iter->getFieldByIndex(slots->idxSlot);
            if (!value::isInt(idxVal)) return false;
            int64_t idx = value::asInt(idxVal);

            if (slots->shape == IteratorShape::ARRAY_ITERATOR)
            {
                const value::Value& arrVal = iter->getFieldByIndex(slots->collSlot);
                if (!value::isNativeArray(arrVal))
                    return false;
                auto arr = value::asNativeArray(arrVal);
                if (!arr) return false;
                *dest = (idx < static_cast<int64_t>(arr->size()));
                return true;
            }
            // LIST_ITERATOR
            const value::Value& sizeVal = iter->getFieldByIndex(slots->sizeSlot);
            if (!value::isInt(sizeVal)) return false;
            *dest = (idx < value::asInt(sizeVal));
            return true;
        }

        bool tryFastNext(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& iter,
                          value::Value* dest)
        {
            auto* classDef = iter->getClassDefinition().get();
            const IteratorFastSlots* slots = resolveSlots(classDef);
            if (!slots) return false;

            if (!iter->hasFieldVector())
                iter->ensureFieldVector();

            const value::Value& idxVal = iter->getFieldByIndex(slots->idxSlot);
            if (!value::isInt(idxVal)) return false;
            int64_t idx = value::asInt(idxVal);

            if (slots->shape == IteratorShape::ARRAY_ITERATOR)
            {
                const value::Value& arrVal = iter->getFieldByIndex(slots->collSlot);
                if (!value::isNativeArray(arrVal))
                    return false;
                auto arr = value::asNativeArray(arrVal);
                if (!arr) return false;
                if (idx < 0 || static_cast<size_t>(idx) >= arr->size()) return false;
                *dest = arr->getUnchecked(static_cast<size_t>(idx));
                iter->setField(*slots->idxFieldName, static_cast<int64_t>(idx + 1));
                return true;
            }
            // LIST_ITERATOR
            const value::Value& sizeVal = iter->getFieldByIndex(slots->sizeSlot);
            const value::Value& dataVal = iter->getFieldByIndex(slots->collSlot);
            if (!value::isInt(sizeVal)) return false;
            if (!value::isNativeArray(dataVal))
                return false;
            auto arr = value::asNativeArray(dataVal);
            if (!arr) return false;
            if (idx < 0 || idx >= value::asInt(sizeVal)) return false;
            if (static_cast<size_t>(idx) >= arr->size()) return false;
            *dest = arr->getUnchecked(static_cast<size_t>(idx));
            iter->setField(*slots->idxFieldName, static_cast<int64_t>(idx + 1));
            return true;
        }
    }

    // GET_ITERATOR: pop collection from callArgs[0], push iterator via *dest.
    void jit_iterator_get(value::Value* dest, JitContext* ctx)
    {
        if (ctx->pendingException) return;
        try
        {
            value::Value& receiver = ctx->callArgs[0];

            if (value::isNullType(receiver) ||
                value::isVoid(receiver))
            {
                throw errors::NullPointerException(
                    "Cannot get iterator from null collection");
            }

            // Array fast path: wrap in ArrayIteratorHelper directly.
            if (value::isNativeArray(receiver))
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
        catch (const OSRDeoptException&)
        {
            throw;
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

            if (value::isNullType(receiver) ||
                value::isVoid(receiver))
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
        catch (const OSRDeoptException&)
        {
            throw;
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

            if (value::isNullType(receiver) ||
                value::isVoid(receiver))
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
        catch (const OSRDeoptException&)
        {
            throw;
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

        if (value::isNullType(receiver) ||
            value::isVoid(receiver))
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
        catch (const OSRDeoptException&)
        {
            // Deopt must reach the call boundary even if close() raises it.
            throw;
        }
        catch (...)
        {
            // Swallow: close() is best-effort cleanup.
        }
    }
}
