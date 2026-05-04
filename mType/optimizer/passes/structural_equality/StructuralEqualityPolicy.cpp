#include "StructuralEqualityPolicy.hpp"

#include "../../../ast/GenericType.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../errors/InheritanceException.hpp"
#include "../../../value/ValueType.hpp"

namespace optimizer::passes::structural_equality
{
    namespace
    {
        // Returns the first method on `node` matching (name, arity).
        // If `skipSynthetic` is true, compiler-generated methods are
        // ignored — used for parent-conflict detection (we want to know
        // whether the user wrote their own contract on the parent, not
        // whether this pass already added one).
        const ast::nodes::classes::MethodNode* findMethod(
            const ast::ClassNode* node, const std::string& name, size_t arity,
            bool skipSynthetic)
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
    }

    bool StructuralEqualityPolicy::declaresHashCode(const ast::ClassNode* node)
    {
        return findMethod(node, "hashCode", 0, /*skipSynthetic*/ true) != nullptr;
    }

    bool StructuralEqualityPolicy::declaresEquals(const ast::ClassNode* node)
    {
        return findMethod(node, "equals", 1, /*skipSynthetic*/ true) != nullptr;
    }

    bool StructuralEqualityPolicy::hasAnyHashCode(const ast::ClassNode* node)
    {
        return findMethod(node, "hashCode", 0, /*skipSynthetic*/ false) != nullptr;
    }

    bool StructuralEqualityPolicy::hasAnyEquals(const ast::ClassNode* node)
    {
        return findMethod(node, "equals", 1, /*skipSynthetic*/ false) != nullptr;
    }

    bool StructuralEqualityPolicy::isShapeSkippable(const ast::ClassNode* node)
    {
        if (node->isAbstract()) return true;
        if (node->isValueClass()) return true;
        if (node->isGeneric()) return true;
        return false;
    }

    std::vector<const ast::FieldNode*>
    StructuralEqualityPolicy::collectOwnInstanceFields(const ast::ClassNode* node)
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

    namespace
    {
        const ast::ClassNode* resolveImmediateParent(
            const ast::ClassNode* node,
            const ClassRegistry& registry)
        {
            if (!node->hasParentClass()) return nullptr;
            const std::string& parentName = node->getParentClassName();
            if (parentName == "Object") return nullptr;
            auto it = registry.find(parentName);
            if (it == registry.end()) return nullptr;
            return it->second;
        }
    }

    bool StructuralEqualityPolicy::parentHasHashCodeMethod(
        const ast::ClassNode* node,
        const ClassRegistry& registry)
    {
        const auto* parent = resolveImmediateParent(node, registry);
        if (!parent) return false;
        return hasAnyHashCode(parent);
    }

    bool StructuralEqualityPolicy::parentHasEqualsMethod(
        const ast::ClassNode* node,
        const ClassRegistry& registry)
    {
        const auto* parent = resolveImmediateParent(node, registry);
        if (!parent) return false;
        return hasAnyEquals(parent);
    }

    bool StructuralEqualityPolicy::allFieldsSafeForSynthesis(
        const std::vector<const ast::FieldNode*>& ownFields)
    {
        // Phase 1: only int-primitive fields are safe. The codegen does
        // direct integer arithmetic on those (no method calls). All other
        // field shapes — string-variant types (class names, interface
        // names, generic parameters), parameterized generics, nullable
        // object fields, and non-int concrete primitives — are rejected.
        // Reasons:
        //   * String-variant could be an interface (no hashCode resolution)
        //     or a class whose hashCode signature might conflict.
        //   * Parameterized types (Array<T>, Promise<T>) don't expose a
        //     callable hashCode in synthesized scope.
        //   * Nullable object fields would require null-aware codegen that
        //     passes nullable args to non-nullable Object equals params.
        //   * Non-int primitives (float/bool/string) don't support
        //     `.hashCode()` directly in mType.
        // Classes failing this gate fall back to the slow Object native —
        // correct, just not accelerated. Phase 1.5+ can relax incrementally.
        for (const auto* field : ownFields)
        {
            auto genericType = field->getGenericType();
            if (!genericType) return false;
            if (genericType->isParameterized()) return false;
            if (genericType->isNullable()) return false;
            if (genericType->isGenericParameter()) return false;

            ::value::ValueType vt = genericType->getConcreteType();
            if (vt != ::value::ValueType::INT) return false;
        }
        return true;
    }

    void StructuralEqualityPolicy::enforceParentConflictRule(
        const ast::ClassNode* node,
        const ClassRegistry& registry)
    {
        const bool childHasHash = declaresHashCode(node);
        const bool childHasEq = declaresEquals(node);
        if (childHasHash && childHasEq) return;  // child fully overrides — fine.

        const ast::ClassNode* current = node;
        while (current && current->hasParentClass())
        {
            const std::string& parentName = current->getParentClassName();
            if (parentName == "Object") return;
            auto it = registry.find(parentName);
            if (it == registry.end()) return;  // unresolved parent — handled elsewhere.

            const auto* parent = it->second;
            const bool parentHasHash = declaresHashCode(parent);
            const bool parentHasEq = declaresEquals(parent);
            if (parentHasHash || parentHasEq)
            {
                throw errors::InheritanceException(
                    "Class '" + node->getClassName() +
                    "' inherits from '" + parent->getClassName() +
                    "' which declares its own hashCode/equals; '" +
                    node->getClassName() +
                    "' must override BOTH explicitly. Auto-synthesizing only "
                    "one (or none) would silently disagree with the parent's "
                    "equals/hashCode contract.",
                    node->getClassName(),
                    parent->getClassName(),
                    node->getLocation());
            }
            current = parent;
        }
    }
}
