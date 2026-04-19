#include "JitHelpers.hpp"
#include "../../value/ValueShim.hpp"
#include "../../gc/GC.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../value/ValueObject.hpp"
#include <new>

namespace vm::jit
{
    namespace
    {
        // Boxed Int / Float primitives always keep their scalar under field index 0;
        // the invariant is asserted in PrimitiveMethodExecutor and PrimitiveTypeTag.
        template <typename T>
        bool tryReadBoxedField(const value::Value& val, T& out)
        {
            // MYT-189: uses holdsT<T>/getT<T> so the helper compiles on both
            // flag paths. The surrounding JIT is still force-disabled flag-on
            // at VirtualMachine::setJitEnabled because JIT-emitted machine
            // code bakes in the std::variant Value layout — the helper just
            // stays linkable and correct if the gate ever lifts.
            if (value::isObject(val))
            {
                const auto& obj = value::asObject(val);
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
}
