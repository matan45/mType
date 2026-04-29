#pragma once
// MYT-228: shared compile-side utilities for detecting whether a type name
// is a generic parameter declared by some enclosing scope (class / method /
// free function). Used by:
//   - ExpressionCompiler::compileInstanceOf / compileCast — to decide
//     whether a bare RHS like `T` should emit *_TYPEPARAM (resolved at
//     runtime) versus a plain class-name lookup.
//   - FunctionCallHelper / ClassCompiler — to tag a BIND_TYPE_ARGS value
//     as forward-from-caller (kind=1) when the value name itself is a
//     type-param in scope at the call site.
//
// The previous implementations duplicated the same three-way scope walk
// in three different places. Centralising it here keeps the decision
// rule consistent if the scope-tracking design ever changes (e.g. nested
// generic functions, super-method generic forwarding, etc.).

#include <string>
#include "CompilerContext.hpp"

namespace vm::compiler::visitors
{
    // Returns true if `name` matches a declared type parameter in any
    // enclosing scope visible at the current compile point. Walks:
    //   1. ctx.currentClassNode->getGenericParameters()
    //   2. ctx.currentMethodNode->getGenericTypeParameters()
    //   3. ctx.currentFunctionNode->getGenericTypeParameters()
    //
    // NOTE: MethodCompilerHelper does NOT push a self-mapping onto
    // ctx.genericTypeBindingStack (only FunctionCompiler does, for free
    // generic functions), so callers that need to detect method-level T
    // MUST consult this helper rather than the binding stack alone.
    bool isTypeParamInScope(const CompilerContext& ctx, const std::string& name);
}
