#include "JitHelpers.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include "../../value/ValueShim.hpp"
#include "../../value/AsyncPromiseValue.hpp"
#include "../../gc/GC.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../value/ValueObject.hpp"
#include "../../value/StringPool.hpp"
#include <new>
#include <string>

namespace vm::jit
{
    // Defined in header (extern). JIT JUMP_BACK / tail-call emit inlines the
    // increment and threshold check; this function only runs once per
    // GC_CHECK_INTERVAL crossings.
    size_t g_jit_gc_poll_counter = 0;

    namespace
    {
        // Boxed Int / Float / Bool primitives always keep their scalar under field index 0;
        // the invariant is asserted in PrimitiveMethodExecutor and PrimitiveTypeTag.
        template <typename T>
        bool tryReadBoxedField(const value::Value& val, T& out)
        {
            // MYT-189: uses holdsT<T>/getT<T> so the helper compiles on both
            // flag paths. The surrounding JIT is still force-disabled flag-on
            // at VirtualMachine::setJitEnabled because JIT-emitted machine
            // code bakes in the std::variant Value layout — the helper just
            // stays linkable and correct if the gate ever lifts.
            // MYT-208: accept STACK_OBJECT — boxed-primitive read works
            // identically on raw or shared_ptr-owned ObjectInstance.
            if (value::isAnyObject(val))
            {
                auto* obj = value::asObjectInstanceRaw(val);
                if (!obj) return false;
                obj->ensureFieldVector();
                const value::Value& field = obj->getFieldByIndex(0);
                if (value::holdsT<T>(field))
                {
                    out = value::getT<T>(field);
                    return true;
                }
                return false;
            }
            if (value::isValueObject(val))
            {
                const auto& obj = value::asValueObject(val);
                if (!obj) return false;
                const value::Value& field = obj->getFieldByIndex(0);
                if (value::holdsT<T>(field))
                {
                    out = value::getT<T>(field);
                    return true;
                }
                return false;
            }
            return false;
        }
    }

    extern "C" {

        int64_t jit_unbox_int(const value::Value* val)
        {
            if (value::isInt(*val))
            {
                return value::asInt(*val);
            }
            if (value::isBool(*val))
            {
                return value::asBool(*val) ? 1 : 0;
            }
            int64_t boxed = 0;
            if (tryReadBoxedField<int64_t>(*val, boxed))
            {
                return boxed;
            }
            bool boxedBool = false;
            if (tryReadBoxedField<bool>(*val, boxedBool))
            {
                return boxedBool ? 1 : 0;
            }
            return 0;
        }

        void jit_set_return_int(JitContext* ctx, int64_t val)
        {
            ctx->returnValue = val;
            ctx->hasReturnValue = true;
        }

        void jit_set_return_bool(JitContext* ctx, int64_t val)
        {
            ctx->returnValue = (val != 0);
            ctx->hasReturnValue = true;
        }

        void jit_gc_safepoint()
        {
            // Reached only when the inline polling counter emitted by JIT
            // JUMP_BACK / tail-call crosses gc::config::GC_CHECK_INTERVAL.
            // Reset the counter here, not on the hot emitted path.
            g_jit_gc_poll_counter = 0;
            gc::GC::maybeCollect();
        }

        double jit_unbox_float(const value::Value* val)
        {
            if (value::isFloat(*val))
            {
                return value::asFloat(*val);
            }
            if (value::isInt(*val))
            {
                return static_cast<double>(value::asInt(*val));
            }
            double dboxed = 0.0;
            if (tryReadBoxedField<double>(*val, dboxed))
            {
                return dboxed;
            }
            int64_t iboxed = 0;
            if (tryReadBoxedField<int64_t>(*val, iboxed))
            {
                return static_cast<double>(iboxed);
            }
            return 0.0;
        }

        void jit_set_return_float(JitContext* ctx, double val)
        {
            ctx->returnValue = val;
            ctx->hasReturnValue = true;
        }

        void jit_box_int(value::Value* dest, int64_t val)
        {
            *dest = val;
        }

        void jit_box_float(value::Value* dest, double val)
        {
            *dest = val;
        }

        void jit_box_bool(value::Value* dest, int64_t val)
        {
            *dest = (val != 0);
        }

        void jit_box_null(value::Value* dest)
        {
            *dest = std::monostate{};
        }

        void jit_locals_init(value::Value* base, size_t count)
        {
            for (size_t i = 0; i < count; i++)
            {
                new (&base[i]) value::Value(std::monostate{});
            }
        }

        void jit_locals_cleanup(value::Value* base, size_t count)
        {
            for (size_t i = 0; i < count; i++)
            {
                std::destroy_at(&base[i]);
            }
        }

        void jit_value_destroy(value::Value* dest)
        {
            std::destroy_at(dest);
            new (dest) value::Value(std::monostate{});
        }

        int64_t jit_values_equal(const value::Value* left, const value::Value* right)
        {
            auto isNull = [](const value::Value& v) {
                return value::isVoid(v) ||
                       value::isNullType(v);
            };

            if (isNull(*left) && isNull(*right)) return 1;
            if (isNull(*left) || isNull(*right)) return 0;
            if (left->tag() != right->tag()) return 0;

            if (value::isInt(*left))
                return value::asInt(*left) == value::asInt(*right) ? 1 : 0;
            if (value::isFloat(*left))
                return value::asFloat(*left) == value::asFloat(*right) ? 1 : 0;
            if (value::isBool(*left))
                return value::asBool(*left) == value::asBool(*right) ? 1 : 0;
            if (value::isString(*left))
                return value::asString(*left) == value::asString(*right) ? 1 : 0;
            if (value::isInternedString(*left))
                return value::asInternedString(*left) == value::asInternedString(*right) ? 1 : 0;

            return 0;
        }

    } // extern "C"

    namespace
    {
        // Read the string content out of a Value of either raw STRING /
        // INTERNED_STRING form, or a boxed String ObjectInstance/ValueObject
        // with field 0 = string. Returns true on success and writes into out;
        // returns false otherwise (caller treats as empty/equal-mismatch).
        bool tryReadString(const value::Value& val, std::string& out)
        {
            if (value::isString(val))
            {
                out = value::asString(val);
                return true;
            }
            if (value::isInternedString(val))
            {
                out = std::string(value::asInternedString(val).getString());
                return true;
            }
            if (value::isAnyObject(val))
            {
                auto* obj = value::asObjectInstanceRaw(val);
                if (!obj) return false;
                obj->ensureFieldVector();
                const value::Value& field = obj->getFieldByIndex(0);
                if (value::isString(field)) { out = value::asString(field); return true; }
                if (value::isInternedString(field))
                {
                    out = std::string(value::asInternedString(field).getString());
                    return true;
                }
                return false;
            }
            if (value::isValueObject(val))
            {
                const auto& obj = value::asValueObject(val);
                if (!obj) return false;
                const value::Value& field = obj->getFieldByIndex(0);
                if (value::isString(field)) { out = value::asString(field); return true; }
                if (value::isInternedString(field))
                {
                    out = std::string(value::asInternedString(field).getString());
                    return true;
                }
                return false;
            }
            return false;
        }
    }

    int64_t jit_invoke_string_length(const value::Value* receiver)
    {
        std::string s;
        if (!tryReadString(*receiver, s)) return 0;
        return static_cast<int64_t>(s.size());
    }

    int64_t jit_invoke_string_is_empty(const value::Value* receiver)
    {
        std::string s;
        if (!tryReadString(*receiver, s)) return 1;
        return s.empty() ? 1 : 0;
    }

    int64_t jit_invoke_string_equals(const value::Value* receiver, const value::Value* arg)
    {
        std::string a, b;
        if (!tryReadString(*receiver, a) || !tryReadString(*arg, b)) return 0;
        return (a == b) ? 1 : 0;
    }

    void jit_invoke_string_concat(value::Value* dest,
                                  const value::Value* receiver,
                                  const value::Value* arg)
    {
        // ALIASING NOTE: the JIT emit case passes the same operand-stack slot
        // address for both `dest` and `receiver` (recvAddr is reused so the
        // result lands on the receiver's slot after the arg pop). The reads
        // through `tryReadString(*receiver, ...)` MUST happen before the
        // `std::destroy_at(dest)` below — `a` and `b` are owning copies, so
        // the in-place destruction is safe ONLY because the source bytes have
        // already been copied out. Do not reorder these two blocks.
        std::string a, b;
        if (!tryReadString(*receiver, a) || !tryReadString(*arg, b))
        {
            std::destroy_at(dest);
            new (dest) value::Value(std::string{});
            return;
        }
        std::string result = std::move(a) + b;
        // Mirror the interpreter handler: try to intern. For cycling concat
        // patterns the pool collapses repeated allocations into refcount bumps.
        value::InternedString interned = value::StringPool::getInstance().intern(result);
        std::destroy_at(dest);  // safe: receiver/arg payloads already copied into a/b above
        if (!interned.empty())
        {
            new (dest) value::Value(std::move(interned));
        }
        else
        {
            new (dest) value::Value(std::move(result));
        }
    }

    void jit_create_promise(JitContext* ctx, value::Value* val)
    {
        // *val may be uninitialized garbage if a previous CALL helper stashed
        // an exception (function-level JIT bodies have no implicit pending-
        // exception check between opcodes). Skip work in that case; the body
        // will fall through to RETURN_VALUE and executeCallWithJit's
        // pendingException rethrow surfaces the original error.
        if (ctx && ctx->pendingException) return;

        if (value::isInt(*val))
        {
            *val = value::Value(value::asInt(*val), value::Value::PromiseIntTag{});
            return;
        }

        auto promise = std::make_shared<value::AsyncPromiseValue>(*val);
        *val = std::shared_ptr<value::PromiseValue>(promise);
    }

    void jit_object_to_value_create_promise(JitContext* ctx, value::Value* val)
    {
        if (ctx && ctx->pendingException) return;

        if (value::isObject(*val))
        {
            const auto& instance = value::asObject(*val);
            if (instance && instance->getPrimitiveTag() == value::PrimitiveTypeTag::INT)
            {
                value::Value field = instance->getFieldValue("value");
                if (value::isInt(field))
                {
                    *val = value::Value(value::asInt(field), value::Value::PromiseIntTag{});
                    return;
                }
            }
        }

        jit_object_to_value(val);
        jit_create_promise(ctx, val);
    }

    // Mirrors VirtualMachine::executeAwait, fast-path arms only.
    // - PROMISE_INT: unwrap inline; rebuild as a plain INT Value.
    // - Heap PROMISE in FULFILLED state: copy out promise->getValue().
    // - Anything else (pending, rejected, non-promise, null): deopt to the
    //   interpreter via OSRDeoptException so the interpreter's existing
    //   suspend / UserException / type-error paths run.
    void jit_await(JitContext* ctx, value::Value* val, uint64_t bytecodeOffset)
    {
        // Skip when an earlier CALL helper stashed an exception — *val may
        // be garbage (the JIT body emits a "push return value" after every
        // cc.invoke regardless of hasReturnValue, and isPromise/isPromiseInt
        // could match by accident on uninitialized memory and crash on a
        // bogus shared_ptr deref).
        if (ctx && ctx->pendingException) return;

        if (value::isPromiseInt(*val))
        {
            *val = value::Value(value::asPromiseIntValue(*val));
            return;
        }

        if (value::isPromise(*val))
        {
            const auto& promise = value::asPromise(*val);
            if (promise && promise->isFulfilled())
            {
                *val = promise->getValue();
                return;
            }
        }

        throw OSRDeoptException(static_cast<size_t>(bytecodeOffset));
    }
}
