#include "ClassRegistrar.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../errors/InheritanceException.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../environment/registry/ClassDefinition.hpp"
#include "../../../environment/registry/MethodDefinition.hpp"
#include "../../../environment/registry/SignatureUtils.hpp"

namespace vm::compiler::registration
{
    namespace
    {
        ast::SourceLocation findMethodLocation(
            ast::ClassNode* classNode,
            const std::string& methodName,
            const ast::SourceLocation& fallback)
        {
            if (!classNode) {
                return fallback;
            }
            for (const auto& methodNodePtr : classNode->getMethods()) {
                auto* methodNode = dynamic_cast<ast::nodes::classes::MethodNode*>(methodNodePtr.get());
                if (methodNode && methodNode->getName() == methodName) {
                    return methodNode->getLocation();
                }
            }
            return fallback;
        }

        std::vector<std::pair<std::string, value::ParameterType>>
        stripThisParameter(const std::vector<std::pair<std::string, value::ParameterType>>& params)
        {
            std::vector<std::pair<std::string, value::ParameterType>> out;
            out.reserve(params.size());
            for (const auto& p : params) {
                if (p.first != "this") {
                    out.push_back(p);
                }
            }
            return out;
        }
    }

    void ClassRegistrar::validateMethodOverrides(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> childClass,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentClass,
        ast::ClassNode* classNode
    ) const
    {
        if (!childClass || !parentClass) {
            return;
        }

        const ast::SourceLocation classLocation = classNode ? classNode->getLocation() : ast::SourceLocation();

        const auto& childMethods = childClass->getInstanceMethods();

        // Object's methods (toString/equals/hashCode) intentionally allow
        // typed overloads — skip invariance against Object below.
        auto classRegistry = environment->getClassRegistry();
        auto objectClassDef = classRegistry ? classRegistry->findClass("Object") : nullptr;

        for (const auto& [methodName, childMethodOverloads] : childMethods) {
            for (const auto& childMethod : childMethodOverloads) {
                // Exclude `this`: parent and child have different `this` types
                // (Base vs Derived), so it doesn't participate in matching.
                auto paramsWithoutThis = stripThisParameter(childMethod->getParameters());

                auto parentMethod = parentClass->findInstanceMethodBySignatureInHierarchy(
                    methodName,
                    paramsWithoutThis
                );

                bool parentHasMethodName = false;
                auto parentMethodsIt = parentClass->getInstanceMethods().find(methodName);
                if (parentMethodsIt != parentClass->getInstanceMethods().end() && !parentMethodsIt->second.empty()) {
                    parentHasMethodName = true;
                }

                // Invariance: if a parent method with the same name+arity has
                // different param types AND no child overload matches its
                // signature, that's a failed override, not a new overload.
                if (parentHasMethodName && !parentMethod) {
                    bool isObjectMethod = false;
                    if (objectClassDef) {
                        auto objectMethods = objectClassDef->getInstanceMethods().find(methodName);
                        if (objectMethods != objectClassDef->getInstanceMethods().end()) {
                            isObjectMethod = true;
                        }
                    }
                    if (!isObjectMethod) {
                        validateOverrideInvariance(
                            childClass, parentClass, methodName,
                            paramsWithoutThis, parentMethodsIt->second,
                            classNode, classLocation);
                    }
                }

                if (parentMethod) {
                    ast::SourceLocation methodLocation = findMethodLocation(classNode, methodName, classLocation);
                    validateOverrideFinal(parentMethod, childClass, parentClass, methodName, paramsWithoutThis, methodLocation);
                    validateOverrideAccessNarrowing(childMethod, parentMethod, childClass, parentClass, methodName, methodLocation);
                    validateOverrideParameterShape(childMethod, parentMethod, childClass, parentClass, methodName, methodLocation);
                    validateOverrideReturnType(childMethod, parentMethod, childClass, parentClass, methodName, methodLocation);
                }
            }
        }
    }

    void ClassRegistrar::validateOverrideInvariance(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> childClass,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentClass,
        const std::string& methodName,
        const std::vector<std::pair<std::string, value::ParameterType>>& paramsWithoutThis,
        const std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>>& parentMethodOverloads,
        ast::ClassNode* classNode,
        const ast::SourceLocation& classLocation
    ) const
    {
        size_t childArity = paramsWithoutThis.size();

        for (const auto& pMethod : parentMethodOverloads) {
            auto parentParamsWithoutThis = stripThisParameter(pMethod->getParametersWithTypes());

            if (parentParamsWithoutThis.size() != childArity) {
                continue;
            }

            bool typesMatch = true;
            bool isGenericSubstitution = false;
            const auto& typeSubstitutionMap = childClass->getParentTypeSubstitutionMap();

            for (size_t i = 0; i < childArity; ++i) {
                std::string pTypeStr = parentParamsWithoutThis[i].second.className.has_value()
                    ? parentParamsWithoutThis[i].second.className.value()
                    : runtimeTypes::klass::SignatureUtils::getTypeName(parentParamsWithoutThis[i].second);

                std::string cTypeStr = paramsWithoutThis[i].second.className.has_value()
                    ? paramsWithoutThis[i].second.className.value()
                    : runtimeTypes::klass::SignatureUtils::getTypeName(paramsWithoutThis[i].second);

                if (pTypeStr != cTypeStr) {
                    typesMatch = false;

                    auto substitutionIt = typeSubstitutionMap.find(pTypeStr);
                    if (substitutionIt != typeSubstitutionMap.end() && substitutionIt->second == cTypeStr) {
                        isGenericSubstitution = true;
                    }

                    if (pTypeStr.find('<') != std::string::npos) {
                        isGenericSubstitution = true;
                    }
                }
            }

            if (typesMatch || isGenericSubstitution) {
                continue;
            }

            bool isOverriddenByAnyChild = false;
            auto childMethodsIt = childClass->getInstanceMethods().find(methodName);
            if (childMethodsIt != childClass->getInstanceMethods().end()) {
                for (const auto& childMethodOverload : childMethodsIt->second) {
                    auto childOverloadParams = stripThisParameter(childMethodOverload->getParametersWithTypes());
                    if (childOverloadParams.size() != parentParamsWithoutThis.size()) {
                        continue;
                    }
                    bool allMatch = true;
                    for (size_t i = 0; i < parentParamsWithoutThis.size(); ++i) {
                        if (!(childOverloadParams[i].second == parentParamsWithoutThis[i].second)) {
                            allMatch = false;
                            break;
                        }
                    }
                    if (allMatch) {
                        isOverriddenByAnyChild = true;
                        break;
                    }
                }
            }

            if (!isOverriddenByAnyChild) {
                ast::SourceLocation methodLocation = findMethodLocation(classNode, methodName, classLocation);
                throw errors::InheritanceException(
                    "Method parameter type invariance violation in class '" + childClass->getName() +
                    "': method '" + methodName + "' has the same number of parameters as a parent method in '" +
                    parentClass->getName() + "' but with different parameter types, and the parent method is not being overridden. " +
                    "This appears to be a failed override attempt. Method parameters are invariant and must match exactly when overriding.",
                    childClass->getName(),
                    parentClass->getName(),
                    methodName,
                    methodLocation
                );
            }
        }
    }

    void ClassRegistrar::validateOverrideFinal(
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> parentMethod,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> childClass,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentClass,
        const std::string& methodName,
        const std::vector<std::pair<std::string, value::ParameterType>>& paramsWithoutThis,
        const ast::SourceLocation& methodLocation
    ) const
    {
        if (!parentMethod->isFinal()) {
            return;
        }

        // Walk hierarchy to find the class that actually defines this final
        // method (parentClass may have inherited it).
        std::string definingClassName = parentClass->getName();
        auto currentClass = parentClass;
        while (currentClass) {
            auto localMethod = currentClass->findInstanceMethodBySignature(methodName, paramsWithoutThis);
            if (localMethod) {
                definingClassName = currentClass->getName();
                break;
            }
            currentClass = currentClass->getParentClass();
        }

        throw errors::InheritanceException(
            "Cannot override final method '" + methodName +
            "' from class '" + definingClassName + "' in class '" + childClass->getName() + "'",
            childClass->getName(),
            definingClassName,
            methodName,
            methodLocation
        );
    }

    void ClassRegistrar::validateOverrideAccessNarrowing(
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> childMethod,
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> parentMethod,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> childClass,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentClass,
        const std::string& methodName,
        const ast::SourceLocation& methodLocation
    ) const
    {
        ast::AccessModifier childAccess = childMethod->getAccessModifier();
        ast::AccessModifier parentAccess = parentMethod->getAccessModifier();

        bool isNarrowing = false;
        if (parentAccess == ast::AccessModifier::PUBLIC && childAccess != ast::AccessModifier::PUBLIC) {
            isNarrowing = true;
        } else if (parentAccess == ast::AccessModifier::PROTECTED && childAccess == ast::AccessModifier::PRIVATE) {
            isNarrowing = true;
        }

        if (!isNarrowing) {
            return;
        }

        throw errors::InheritanceException(
            "Method override access modifier narrowing in class '" + childClass->getName() +
            "': method '" + methodName + "' cannot narrow access from '" +
            ast::accessModifierToString(parentAccess) + "' to '" +
            ast::accessModifierToString(childAccess) + "'",
            childClass->getName(),
            parentClass->getName(),
            methodName,
            methodLocation
        );
    }

    void ClassRegistrar::validateOverrideParameterShape(
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> childMethod,
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> parentMethod,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> childClass,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentClass,
        const std::string& methodName,
        const ast::SourceLocation& methodLocation
    ) const
    {
        const auto& childParams = childMethod->getParameters();
        const auto& parentParams = parentMethod->getParameters();

        if (childParams.size() != parentParams.size()) {
            throw errors::InheritanceException(
                "Method override signature mismatch in class '" + childClass->getName() +
                "': method '" + methodName + "' has " + std::to_string(childParams.size()) +
                " parameters but parent method has " + std::to_string(parentParams.size()) + " parameters",
                childClass->getName(),
                parentClass->getName(),
                methodName,
                methodLocation
            );
        }

        // Skip `this` at index 0 — it's always different between parent and child.
        const auto& typeSubstitutionMap = childClass->getParentTypeSubstitutionMap();

        for (size_t i = 1; i < childParams.size(); ++i) {
            const auto& childParam = childParams[i].second;
            const auto& parentParam = parentParams[i].second;

            // Apply T → ConcreteType substitution to parent's declared type
            // before comparison (e.g., parent param "T", child class extends
            // Parent<String>, child param "String").
            std::string parentParamTypeName = parentParam.toString();
            auto it = typeSubstitutionMap.find(parentParamTypeName);
            if (it != typeSubstitutionMap.end()) {
                parentParamTypeName = it->second;
            }

            std::string childParamTypeName = childParam.toString();

            if (childParamTypeName != parentParamTypeName) {
                throw errors::InheritanceException(
                    "Method override parameter type mismatch in class '" + childClass->getName() +
                    "': method '" + methodName + "' parameter " + std::to_string(i) +
                    " has type '" + childParamTypeName +
                    "' but parent method expects '" + parentParamTypeName + "'",
                    childClass->getName(),
                    parentClass->getName(),
                    methodName,
                    methodLocation
                );
            }
        }
    }

    void ClassRegistrar::validateOverrideReturnType(
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> childMethod,
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> parentMethod,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> childClass,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentClass,
        const std::string& methodName,
        const ast::SourceLocation& methodLocation
    ) const
    {
        if (childMethod->getReturnType() == parentMethod->getReturnType()) {
            return;
        }

        throw errors::InheritanceException(
            "Method override return type mismatch in class '" + childClass->getName() +
            "': method '" + methodName + "' has return type '" +
            ::types::TypeConversionUtils::getTypeDisplayName(childMethod->getReturnType()) +
            "' but parent method has return type '" +
            ::types::TypeConversionUtils::getTypeDisplayName(parentMethod->getReturnType()) + "'",
            childClass->getName(),
            parentClass->getName(),
            methodName,
            methodLocation
        );
    }
}
