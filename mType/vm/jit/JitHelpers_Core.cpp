#include "JitHelpers.hpp"
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
            if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val))
            {
                const auto& obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
                if (!obj) return false;
                obj->ensureFieldVector();
                const value::Value& field = obj->getFieldByIndex(0);
                if (std::holds_alternative<T>(field))
                {
                    out = std::get<T>(field);
                    return true;
                }
                return false;
            }
            if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(val))
            {
                const auto& obj = std::get<std::shared_ptr<value::ValueObject>>(val);
                if (!obj) return false;
                const value::Value& field = obj->getFieldByIndex(0);
                if (std::holds_alternative<T>(field))
                {
                    out = std::get<T>(field);
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
            if (std::holds_alternative<int64_t>(*val))
            {
                return std::get<int64_t>(*val);
            }
            if (std::holds_alternative<bool>(*val))
            {
                return std::get<bool>(*val) ? 1 : 0;
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
            if (std::holds_alternative<double>(*val))
            {
                return std::get<double>(*val);
            }
            if (std::holds_alternative<int64_t>(*val))
            {
                return static_cast<double>(std::get<int64_t>(*val));
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
                return std::holds_alternative<std::monostate>(v) ||
                       std::holds_alternative<nullptr_t>(v);
            };

            if (isNull(*left) && isNull(*right)) return 1;
            if (isNull(*left) || isNull(*right)) return 0;
            if (left->index() != right->index()) return 0;

            if (std::holds_alternative<int64_t>(*left))
                return std::get<int64_t>(*left) == std::get<int64_t>(*right) ? 1 : 0;
            if (std::holds_alternative<double>(*left))
                return std::get<double>(*left) == std::get<double>(*right) ? 1 : 0;
            if (std::holds_alternative<bool>(*left))
                return std::get<bool>(*left) == std::get<bool>(*right) ? 1 : 0;
            if (std::holds_alternative<std::string>(*left))
                return std::get<std::string>(*left) == std::get<std::string>(*right) ? 1 : 0;
            if (std::holds_alternative<value::InternedString>(*left))
                return std::get<value::InternedString>(*left) == std::get<value::InternedString>(*right) ? 1 : 0;

            return 0;
        }

    } // extern "C"
}
