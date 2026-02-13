#pragma once
#include "../OSRState.hpp"
#include "../JitContext.hpp"
#include "../../runtime/context/ExecutionContext.hpp"
#include <cstddef>
#include <cstdint>
#include <exception>

namespace vm::jit
{
    // Exception thrown by JIT code to trigger deoptimization back to interpreter.
    // Must be in a shared header so throw (JitHelpers.cpp) and catch (OSRManager.cpp)
    // see the same type.
    struct OSRDeoptException : public std::exception
    {
        size_t bytecodeOffset;
        explicit OSRDeoptException(size_t offset) : bytecodeOffset(offset) {}
        const char* what() const noexcept override { return "OSR deoptimization"; }
    };
}

namespace vm::jit::guards
{
    enum class DeoptReason : uint8_t
    {
        TYPE_MISMATCH,
        GUARD_FAILURE,
        EXCEPTION_THROWN,
        UNSUPPORTED_OPCODE
    };

    struct DeoptState
    {
        DeoptReason reason = DeoptReason::EXCEPTION_THROWN;
        size_t bytecodeOffset = 0;
        std::vector<value::Value> locals;
    };

    class DeoptimizationHandler
    {
    public:
        // Restore interpreter state from a deoptimization event
        static bool handleDeopt(const DeoptState& deoptState,
                               const OSRState& osrState,
                               vm::runtime::ExecutionContext& context);
    };
}
