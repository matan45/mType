#pragma once

#include "../../../ast/NodeClassesDeclaration.hpp"
#include <memory>
#include <vector>

namespace optimizer::passes::lombok
{
    // Builds AST nodes for Lombok-synthesized members: getters, setters,
    // toString, and constructors. All returned MethodNodes are marked
    // synthetic so reflection filters them, matching MYT-274. Builder-class
    // synthesis lives in LombokBuilderCodegen.
    class LombokCodegen
    {
    public:
        // `public <T> get<Field>() { return this.<field>; }`
        static std::unique_ptr<ast::MethodNode> buildGetter(const ast::FieldNode* field);

        // `public void set<Field>(<T> value) { this.<field> = value; }`
        static std::unique_ptr<ast::MethodNode> buildSetter(const ast::FieldNode* field);

        // `public string toString() { return "<Class>(a=" + this.a + ...)"; }`
        // `allFields` are the inherited-then-own instance fields to include.
        static std::unique_ptr<ast::MethodNode> buildToString(
            const ast::ClassNode* owner,
            const std::vector<const ast::FieldNode*>& allFields);

        // `public constructor() { }` (with `: super()` when the class has an
        // in-program parent that needs an explicit chain-up).
        static std::unique_ptr<ast::ConstructorNode> buildNoArgsConstructor(bool hasParent);

        // All-args constructor. `inheritedFields` are forwarded to a
        // `super(...)` initializer; `ownFields` are assigned to `this`.
        // Parameter order is inherited-then-own.
        static std::unique_ptr<ast::ConstructorNode> buildAllArgsConstructor(
            const std::vector<const ast::FieldNode*>& inheritedFields,
            const std::vector<const ast::FieldNode*>& ownFields);

        // Required-args constructor for @Data: parameters/assignments for the
        // given (own, final) fields only. No super forwarding.
        static std::unique_ptr<ast::ConstructorNode> buildArgsConstructor(
            const std::vector<const ast::FieldNode*>& fields);
    };
}
