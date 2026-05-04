#pragma once

#include "../../../ast/NodeClassesDeclaration.hpp"
#include <memory>
#include <vector>

namespace optimizer::passes::structural_equality
{
    // MYT-274: builds AST `MethodNode`s for synthesized hashCode() and
    // equals(Object) bodies. The generated bodies follow the Effective Java
    // pattern (h = 31*h + f.hashCode()) and an `isClassOf`-guarded structural
    // equals. Bodies target the inliner's INLINE_SIZE_LIMIT = 32 op budget for
    // ≤4-field classes so HashMap/HashSet hot paths inline them at MONO/POLY
    // call sites.
    class StructuralEqualityCodegen
    {
    public:
        // Builds a public hashCode(): int method whose body hashes the
        // class's OWN instance fields. If `composeWithSuper` is true, the
        // accumulator is initialized from `super.hashCode()` instead of 1,
        // delegating parent-field hashing to the parent's method (avoids
        // the private-field access problem when synthesizing for a child
        // class). The returned MethodNode is marked synthetic so reflection
        // filters it.
        static std::unique_ptr<ast::MethodNode> buildHashCodeMethod(
            const ast::ClassNode* owner,
            const std::vector<const ast::FieldNode*>& ownFields,
            bool composeWithSuper);

        // Builds a public equals(Object other): bool method whose body
        // checks (other isClassOf <ClassName>) and field-by-field equality
        // across the class's OWN fields. If `composeWithSuper` is true,
        // emits `if (!super.equals(other)) return false;` after the
        // instanceof guard — delegating parent-field equality to the
        // parent's method. Returns synthetic MethodNode.
        static std::unique_ptr<ast::MethodNode> buildEqualsMethod(
            const ast::ClassNode* owner,
            const std::vector<const ast::FieldNode*>& ownFields,
            bool composeWithSuper);
    };
}
