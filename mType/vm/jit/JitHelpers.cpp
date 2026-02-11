#include "JitHelpers.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../gc/GC.hpp"

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
            // Fallback for unexpected types
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
    }

    void jit_throw_div_by_zero()
    {
        throw errors::RuntimeException("Division by zero");
    }
}
