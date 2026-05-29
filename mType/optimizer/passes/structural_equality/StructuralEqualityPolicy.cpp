#include "StructuralEqualityPolicy.hpp"
#include <cstddef>

#include "../../../ast/GenericType.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../errors/InheritanceException.hpp"
#include "../../../value/ValueType.hpp"
#include "../shared/ClassShapePolicy.hpp"

namespace optimizer::passes::structural_equality
{
    namespace
    {
        // Method lookup shared with the Lombok pass (single definition in
        // shared::findMethod) so both passes agree on member detection.
        using shared::findMethod;
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
        return shared::isSkippableShape(node);
    }

    std::vector<const ast::FieldNode*>
    StructuralEqualityPolicy::collectOwnInstanceFields(const ast::ClassNode* node)
    {
        return shared::collectOwnInstanceFields(node);
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
        const std::vector<const ast::FieldNode*>& ownFields,
        bool annotationRequested)
    {
        for (const auto* field : ownFields)
        {
            auto genericType = field->getGenericType();
            if (!genericType) return false;

            // Parameterized generics (Array<T>, Promise<T>, collection types)
            // have no guaranteed value-equality contract in either tier.
            if (genericType->isParameterized()) return false;

            if (!annotationRequested)
            {
                // Automatic MYT-274 synthesis: int-primitive fields only.
                // The codegen does direct integer arithmetic on those (no
                // method dispatch). Everything else falls back to the slow
                // Object native so we never silently change value-equality
                // semantics for a class the user never opted into.
                if (genericType->isNullable()) return false;
                if (genericType->isGenericParameter()) return false;
                if (genericType->getConcreteType() != ::value::ValueType::INT) return false;
                continue;
            }

            // Opt-in via @Data / @EqualsAndHashCode: the pre-staged codegen
            // handles class/interface references (string-variant types) by
            // dispatching `.equals()` / `.hashCode()` through Object, with null
            // guards — so accept them regardless of nullability.
            if (genericType->isGenericParameter()) continue;

            ::value::ValueType vt = genericType->getConcreteType();
            const bool supported =
                vt == ::value::ValueType::INT || vt == ::value::ValueType::FLOAT ||
                vt == ::value::ValueType::BOOL || vt == ::value::ValueType::STRING ||
                vt == ::value::ValueType::OBJECT;
            if (!supported) return false;

            // The codegen's null-guard path is OBJECT-only (STRING classifies
            // as OBJECT for codegen). A nullable INT/FLOAT/BOOL would slip
            // through the fast `!=` / arithmetic path with no null handling,
            // so keep those unsupported.
            if (genericType->isNullable() &&
                vt != ::value::ValueType::OBJECT && vt != ::value::ValueType::STRING)
            {
                return false;
            }
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
        size_t depth = 0;
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
                const std::string relation =
                    depth == 0 ? "inherits from" : "transitively inherits from";
                throw errors::InheritanceException(
                    "Class '" + node->getClassName() +
                    "' " + relation + " '" + parent->getClassName() +
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
            ++depth;
        }
    }
}
