#include "LombokPolicy.hpp"
#include "LombokAstBuilders.hpp"

#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"

#include <cstddef>

namespace optimizer::passes::lombok
{
    using ast::nodes::classes::ConstructorNode;
    using ast::nodes::classes::FieldNode;
    using ast::nodes::classes::MethodNode;

    bool LombokPolicy::isShapeSkippable(const ast::ClassNode* node)
    {
        if (node->isAbstract()) return true;
        if (node->isValueClass()) return true;
        if (node->isGeneric()) return true;
        return false;
    }

    bool LombokPolicy::hasMethod(const ast::ClassNode* node, const std::string& name, size_t arity)
    {
        for (const auto& methodAst : node->getMethods())
        {
            auto* method = dynamic_cast<const MethodNode*>(methodAst.get());
            if (!method) continue;
            if (method->getName() != name) continue;
            if (method->getParameterCount() != arity) continue;
            return true;
        }
        return false;
    }

    bool LombokPolicy::hasConstructorWithSignature(
        const ast::ClassNode* node,
        const std::vector<value::ParameterType>& paramTypes)
    {
        for (const auto& ctorAst : node->getConstructors())
        {
            auto* ctor = dynamic_cast<const ConstructorNode*>(ctorAst.get());
            if (!ctor) continue;
            const auto& params = ctor->getParametersWithTypes();
            if (params.size() != paramTypes.size()) continue;

            bool allMatch = true;
            for (size_t i = 0; i < params.size(); ++i)
            {
                if (!(params[i].second == paramTypes[i]))
                {
                    allMatch = false;
                    break;
                }
            }
            if (allMatch) return true;
        }
        return false;
    }

    bool LombokPolicy::hasNoArgConstructor(const ast::ClassNode* node)
    {
        for (const auto& ctorAst : node->getConstructors())
        {
            auto* ctor = dynamic_cast<const ConstructorNode*>(ctorAst.get());
            if (!ctor) continue;
            if (ctor->getParameterCount() == 0) return true;
        }
        return false;
    }

    std::vector<const ast::FieldNode*>
    LombokPolicy::collectOwnInstanceFields(const ast::ClassNode* node)
    {
        std::vector<const ast::FieldNode*> result;
        for (const auto& fieldAst : node->getFields())
        {
            auto* field = dynamic_cast<const FieldNode*>(fieldAst.get());
            if (!field) continue;
            if (field->getIsStatic()) continue;
            result.push_back(field);
        }
        return result;
    }

    std::vector<const ast::FieldNode*>
    LombokPolicy::collectInheritedInstanceFields(
        const ast::ClassNode* node, const ClassRegistry& registry)
    {
        // Walk up collecting each ancestor's own fields, then reverse so the
        // ordering runs ancestor-first (matches constructor parameter order
        // where parent fields precede the child's own fields).
        std::vector<std::vector<const ast::FieldNode*>> perLevel;
        const ast::ClassNode* current = node;
        while (current && current->hasParentClass())
        {
            const std::string& parentName = current->getParentClassName();
            if (parentName == "Object") break;
            auto it = registry.find(parentName);
            if (it == registry.end()) break;   // cross-module parent: stop here.
            const ast::ClassNode* parent = it->second;
            perLevel.push_back(collectOwnInstanceFields(parent));
            current = parent;
        }

        std::vector<const ast::FieldNode*> result;
        for (auto level = perLevel.rbegin(); level != perLevel.rend(); ++level)
        {
            for (const auto* f : *level) result.push_back(f);
        }
        return result;
    }

    std::vector<value::ParameterType> LombokPolicy::toParameterTypes(
        const std::vector<const ast::FieldNode*>& fields)
    {
        std::vector<value::ParameterType> types;
        types.reserve(fields.size());
        for (const auto* field : fields)
        {
            types.push_back(detail::fieldToParameterType(field));
        }
        return types;
    }
}
