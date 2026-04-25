#pragma once
#include "../NativeRegistry.hpp"

namespace environment::registry::builtin
{
    /**
     * Register all language built-in native functions on the given registry.
     * Replaces the per-class BuiltinFunction registration pattern; each function
     * is now bound through NativeBinder so the boilerplate (arity check + variant
     * unwrap + return-value wrap) is handled by the binder rather than per-function.
     */
    void registerAll(NativeRegistry& registry, MethodCallHandler printHandler);
}
