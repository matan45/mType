#pragma once

#include <cstddef>

namespace constants {
    namespace call {
        // Inline capacity of SmallArgsBuffer used by interpreter method/function
        // dispatch paths. Calls with argCount <= this threshold avoid a heap
        // allocation for the args buffer. Larger calls spill to unique_ptr.
        //
        // Chosen empirically: sizeof(value::Value) == 16 so the inline buffer
        // is 128 bytes per call frame. Raising this bloats stack per call.
        // The JIT's JitContext::MAX_CALL_ARGS is independent and covers every
        // JIT-compiled call by design (JIT bails out above its ceiling).
        constexpr size_t INLINE_ARGS_CAPACITY = 8;
    }
}
