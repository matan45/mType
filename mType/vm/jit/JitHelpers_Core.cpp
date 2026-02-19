#include "JitHelpers.hpp"
#include "../../gc/GC.hpp"
#include <new>

namespace vm::jit
{
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

        float jit_unbox_float(const value::Value* val)
        {
            if (std::holds_alternative<float>(*val))
            {
                return std::get<float>(*val);
            }
            if (std::holds_alternative<int64_t>(*val))
            {
                return static_cast<float>(std::get<int64_t>(*val));
            }
            return 0.0f;
        }

        void jit_set_return_float(JitContext* ctx, float val)
        {
            ctx->returnValue = val;
            ctx->hasReturnValue = true;
        }

        void jit_box_int(value::Value* dest, int64_t val)
        {
            *dest = val;
        }

        void jit_box_float(value::Value* dest, float val)
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
            if (std::holds_alternative<float>(*left))
                return std::get<float>(*left) == std::get<float>(*right) ? 1 : 0;
            if (std::holds_alternative<bool>(*left))
                return std::get<bool>(*left) == std::get<bool>(*right) ? 1 : 0;

            return 0;
        }

    } // extern "C"
}
