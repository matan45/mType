#pragma once
// MYT-228: shared runtime helper for resolving a type-parameter name
// against a single CallFrame. Used by:
//   - TypeExecutor::resolveTypeParameter — full call-stack walk; checks
//     each frame in turn via this helper.
//   - TypeExecutor::handleBindTypeArgs — forward-from-caller resolution
//     for valueKind=ForwardFromCaller; consults the caller frame only
//     (callStack.back() before pushCallFrame).
//   - JIT helpers in vm/jit/JitHelpers_Objects.cpp (jit_cast_typeparam,
//     jit_instanceof_typeparam, jit_bind_type_args) — same logic, called
//     from the JIT-emitted helper sites.
//
// The previous implementation duplicated this two-source walk in five
// places. Centralising it here keeps the precedence rule consistent
// (per-frame typeArgBindings beats receiver class-level bindings).

#include <string>
#include <string_view>
#include "../context/ExecutionContext.hpp"

namespace vm::runtime::utils
{
    // Resolve `paramName` against a single frame's bindings.
    //
    // Precedence:
    //   1. frame.typeArgBindings (method/free-fn generic bindings staged
    //      by BIND_TYPE_ARGS) — innermost-wins.
    //   2. frame.getThisInstanceRaw()->getGenericTypeBindings() (class-
    //      level reified bindings on the receiver).
    //
    // Returns:
    //   - Pointer to the resolved concrete type name (owned by the
    //     binding map; stable for the frame's lifetime), or
    //   - nullptr if no binding for `paramName` exists in either source.
    //
    // The returned pointer must NOT be retained past the frame's
    // teardown — copy the string into your own storage if you need
    // longer lifetime.
    inline const std::string* resolveTypeParamInFrame(
        const CallFrame& frame,
        const std::string& paramName) noexcept
    {
        if (frame.typeArgBindings)
        {
            const auto& tab = *frame.typeArgBindings;
            auto it = tab.find(paramName);
            if (it != tab.end() && !it->second.empty())
            {
                return &it->second;
            }
        }

        if (auto* rawThis = frame.getThisInstanceRaw())
        {
            const auto& bindings = rawThis->getGenericTypeBindings();
            auto it = bindings.find(paramName);
            if (it != bindings.end() && !it->second.empty())
            {
                return &it->second;
            }
        }

        return nullptr;
    }
}
