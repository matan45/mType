#pragma once

#include "../../../ast/NodeClassesDeclaration.hpp"
#include <memory>
#include <string>
#include <vector>

namespace optimizer::passes::lombok
{
    // Builds the companion `<Class>Builder` class and the static `builder()`
    // factory method for @Builder. The builder mirrors every instance field of
    // the target class with a fluent setter and a `build()` that constructs the
    // target via its all-args constructor (which the pass ensures exists).
    class LombokBuilderCodegen
    {
    public:
        static std::string builderClassName(const std::string& ownerName)
        {
            return ownerName + "Builder";
        }

        // The sibling `<Class>Builder` class (added as a top-level statement).
        // `allFields` are the inherited-then-own fields, matching the order of
        // the target's all-args constructor.
        static std::unique_ptr<ast::ClassNode> buildBuilderClass(
            const ast::ClassNode* owner,
            const std::vector<const ast::FieldNode*>& allFields);

        // `public static <Class>Builder builder() { return new <Class>Builder(); }`
        static std::unique_ptr<ast::MethodNode> buildBuilderFactory(
            const ast::ClassNode* owner);
    };
}
