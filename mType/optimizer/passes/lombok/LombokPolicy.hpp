#pragma once

#include "../../../ast/NodeClassesDeclaration.hpp"
#include "../../../value/ParameterType.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace optimizer::passes::lombok
{
    // Eligibility rules, member-existence checks, and inheritance-aware field
    // collection for the Lombok synthesis pass. Mirrors StructuralEqualityPolicy
    // in spirit: a class-name -> ClassNode* registry of all in-program classes
    // enables parent-chain walks; cross-module parents (name absent from the
    // registry) cause field collection to fall back to own-fields-only.
    using ClassRegistry = std::unordered_map<std::string, ast::ClassNode*>;

    class LombokPolicy
    {
    public:
        // Skip-by-class-shape: abstract / value / generic. Matches MYT-274;
        // codegen for these shapes is out of Phase 1 scope.
        static bool isShapeSkippable(const ast::ClassNode* node);

        // True iff the class already declares ANY method named `name` with the
        // given arity (synthetic or user-written). Used so synthesis never
        // overwrites an existing member and stays idempotent under the
        // optimizer's fixed-point loop.
        static bool hasMethod(const ast::ClassNode* node, const std::string& name, size_t arity);

        // True iff the class already declares a constructor whose parameter
        // types match `paramTypes` exactly (arity + element-wise ParameterType).
        static bool hasConstructorWithSignature(
            const ast::ClassNode* node,
            const std::vector<value::ParameterType>& paramTypes);

        // True iff the class already declares any zero-argument constructor.
        static bool hasNoArgConstructor(const ast::ClassNode* node);

        // Own declared non-static fields, in declaration order.
        static std::vector<const ast::FieldNode*> collectOwnInstanceFields(
            const ast::ClassNode* node);

        // Inherited non-static fields gathered by walking the parent chain
        // through the in-program registry, ordered from the most-distant
        // ancestor down to the immediate parent (so they precede own fields in
        // an all-args constructor). Stops at Object or the first cross-module
        // (unresolved) parent.
        static std::vector<const ast::FieldNode*> collectInheritedInstanceFields(
            const ast::ClassNode* node, const ClassRegistry& registry);

        // Maps a field list to constructor parameter types via the field's
        // declared GenericType.
        static std::vector<value::ParameterType> toParameterTypes(
            const std::vector<const ast::FieldNode*>& fields);
    };
}
