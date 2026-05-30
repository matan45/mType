#pragma once

#include <string>

namespace ast::nodes::classes
{
    class ClassNode;
}

// Pure text generation for the "Generate lifecycle methods" code action,
// offered on classes annotated with @Script. Kept free of LSP types and
// DocumentManager so the codegen is unit-testable in isolation and so
// CodeActionHandler.cpp stays thin -- mirrors AccessorCodegen.
//
// A concrete @Script class must declare onStart()/onUpdate(float)/onDestroy()
// and a no-arg constructor (enforced by AnnotationValidator_Constraints).
// This module scaffolds whatever is missing, never regenerating a member the
// class already declares. Output matches mType's stdlib style (4-space
// indent) and the canonical signatures the validator reports.
namespace mtype::lsp::lifecyclegen {

// Duplicate detection -- scans the class's own declared methods and returns
// true when one named `name` already exists (signature ignored: lifecycle
// methods are singletons, so name presence alone suppresses regeneration).
bool hasMethod(const ast::nodes::classes::ClassNode& cls, const std::string& name);

// Block of every missing member among the no-arg constructor and the three
// lifecycle methods (onStart/onUpdate/onDestroy), each ending with its
// closing-brace line and separated by a blank line. Returns "" when the
// class already declares all four, so the caller can skip the action.
std::string buildLifecycleBody(const ast::nodes::classes::ClassNode& cls);

} // namespace mtype::lsp::lifecyclegen
