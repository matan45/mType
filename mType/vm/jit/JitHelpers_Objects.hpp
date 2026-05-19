#pragma once
#include "JitContext.hpp"
#include <cstddef>
#include <string>

namespace vm::jit
{
    // Defined in JitHelpers_ObjectCalls.cpp. Called from jit_call_method
    // (same TU) and jit_call_method_ic (in JitHelpers_ObjectCallIC.cpp).
    bool trySpecializedCollectionCall(JitContext* ctx,
                                       const std::string& rawMethodName,
                                       size_t argCount);
}
