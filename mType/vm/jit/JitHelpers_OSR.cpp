#include "JitHelpers.hpp"
#include "guards/DeoptimizationHandler.hpp"

namespace vm::jit
{
    extern "C" {

        void jit_osr_write_local(JitContext* ctx, size_t slot, const value::Value* val)
        {
            if (ctx->osrOutputLocals && slot < ctx->osrLocalCount)
            {
                ctx->osrOutputLocals[slot] = *val;
            }
        }

        void jit_osr_exit(JitContext* ctx, uint64_t exitOffset)
        {
            ctx->osrExitOffset = static_cast<size_t>(exitOffset);
            ctx->osrExited = true;
        }

    } // extern "C"

    void jit_osr_deoptimize(JitContext* ctx, uint64_t bytecodeOffset)
    {
        throw OSRDeoptException(static_cast<size_t>(bytecodeOffset));
    }
}
