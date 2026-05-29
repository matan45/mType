#pragma once

// Shared class-shape predicates used by BOTH synthesis passes
// (LombokSynthesisPass via LombokPolicy and StructuralEqualitySynthesisPass
// via StructuralEqualityPolicy). The two passes MUST observe the same notion
// of "skippable shape" and "own instance fields" — otherwise the constructor
// parameter set Lombok emits could diverge from the field set StructuralEquality
// hashes. Header-only `inline` functions so both translation units (including
// the LSP, which compiles LombokPolicy.cpp) share one definition with no extra
// .cpp/link/premake wiring.

#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace optimizer::passes::shared
{
    // Class shapes both synthesis passes skip: abstract / value / generic.
    inline bool isSkippableShape(const ast::ClassNode* node)
    {
        return node->isAbstract() || node->isValueClass() || node->isGeneric();
    }

    // First method on `node` matching (name, arity). When `skipSynthetic` is
    // true, compiler-generated methods are ignored — used for parent-conflict
    // detection, which wants to know whether the user wrote their own contract,
    // not whether a pass already synthesized one.
    inline const ast::nodes::classes::MethodNode* findMethod(
        const ast::ClassNode* node, const std::string& name, std::size_t arity,
        bool skipSynthetic = false)
    {
        for (const auto& methodAst : node->getMethods())
        {
            auto* method = dynamic_cast<const ast::nodes::classes::MethodNode*>(methodAst.get());
            if (!method) continue;
            if (skipSynthetic && method->isSynthetic()) continue;
            if (method->getName() != name) continue;
            if (method->getParameterCount() != arity) continue;
            return method;
        }
        return nullptr;
    }

    // True iff the class already declares ANY method named `name` with the
    // given arity (synthetic or user-written).
    inline bool hasMethod(const ast::ClassNode* node, const std::string& name, std::size_t arity)
    {
        return findMethod(node, name, arity, /*skipSynthetic*/ false) != nullptr;
    }

    // Own declared non-static fields, in declaration order.
    inline std::vector<const ast::FieldNode*> collectOwnInstanceFields(const ast::ClassNode* node)
    {
        std::vector<const ast::FieldNode*> result;
        for (const auto& fieldAst : node->getFields())
        {
            auto* field = dynamic_cast<const ast::FieldNode*>(fieldAst.get());
            if (!field) continue;
            if (field->getIsStatic()) continue;
            result.push_back(field);
        }
        return result;
    }
}
