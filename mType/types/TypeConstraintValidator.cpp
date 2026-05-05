#include "TypeConstraintValidator.hpp"
#include <cstddef>
#include "../ast/GenericTypeParameter.hpp"
#include "../environment/registry/ClassRegistry.hpp"
#include "../runtimeTypes/klass/InterfaceRegistry.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/InterfaceDefinition.hpp"
#include <sstream>
#include <algorithm>

namespace types
{
    std::string ValidationResult::getErrorMessage() const
    {
        if (errors.empty())
        {
            return "";
        }

        std::ostringstream oss;
        for (size_t i = 0; i < errors.size(); ++i)
        {
            if (i > 0) oss << "; ";
            oss << errors[i];
        }
        return oss.str();
    }

    TypeConstraintValidator::TypeConstraintValidator(
        std::shared_ptr<environment::registry::ClassRegistry> classReg,
        std::shared_ptr<runtimeTypes::klass::InterfaceRegistry> interfaceReg)
        : classRegistry(std::move(classReg))
        , interfaceRegistry(std::move(interfaceReg))
    {
    }

    ValidationResult TypeConstraintValidator::validateTypeArguments(
        const std::vector<ast::GenericTypeParameter>& parameters,
        const std::vector<UnifiedTypePtr>& arguments) const
    {
        if (parameters.size() != arguments.size())
        {
            std::ostringstream oss;
            oss << "Type argument count mismatch: expected " << parameters.size()
                << " but got " << arguments.size();
            return ValidationResult::failure(oss.str());
        }

        ValidationResult result;

        for (size_t i = 0; i < parameters.size(); ++i)
        {
            const auto& param = parameters[i];
            const auto& arg = arguments[i];

            if (!arg)
            {
                result.addError("Null type argument for parameter '" + param.name + "'");
                continue;
            }

            // Check each constraint
            for (const auto& constraintStr : param.constraints)
            {
                auto constraintResult = validateSingleConstraint(param.name, arg, constraintStr);
                if (!constraintResult.isValid())
                {
                    for (const auto& err : constraintResult.getErrors())
                    {
                        result.addError(err);
                    }
                }
            }
        }

        return result;
    }

    ValidationResult TypeConstraintValidator::validateSingleConstraint(
        const std::string& paramName,
        const UnifiedTypePtr& argument,
        const std::string& constraintStr) const
    {
        std::string constraintType = extractConstraintTypeName(constraintStr);

        if (constraintType.empty())
        {
            return ValidationResult::success();
        }

        // Check if the argument type satisfies the constraint
        bool satisfied = false;

        // First check if it's an interface constraint
        if (interfaceRegistry && interfaceRegistry->hasInterface(constraintType))
        {
            satisfied = implementsInterface(argument, constraintType);
        }
        // Then check if it's a class constraint
        else if (classRegistry && classRegistry->hasClass(constraintType))
        {
            satisfied = extendsClass(argument, constraintType);
        }
        else
        {
            // Unknown constraint type - could be a forward reference
            // For now, we'll accept it and let runtime catch errors
            return ValidationResult::success();
        }

        if (!satisfied)
        {
            std::ostringstream oss;
            oss << "Type argument '" << argument->toString()
                << "' for parameter '" << paramName
                << "' does not satisfy constraint '" << constraintStr << "'";
            return ValidationResult::failure(oss.str());
        }

        return ValidationResult::success();
    }

    bool TypeConstraintValidator::satisfiesConstraint(
        const UnifiedTypePtr& concreteType,
        const TypeConstraint& constraint) const
    {
        if (!concreteType || !constraint.boundType)
        {
            return false;
        }

        const std::string& boundName = constraint.boundType->getName();

        switch (constraint.kind)
        {
            case TypeConstraint::Kind::Extends:
                return extendsClass(concreteType, boundName);

            case TypeConstraint::Kind::Implements:
                return implementsInterface(concreteType, boundName);

            default:
                return false;
        }
    }

    bool TypeConstraintValidator::extendsClass(
        const UnifiedTypePtr& type,
        const std::string& baseClassName) const
    {
        if (!type || !classRegistry)
        {
            return false;
        }

        // Get the type name
        std::string typeName = type->getName();

        // Same class - trivially extends
        if (typeName == baseClassName)
        {
            return true;
        }

        // Look up the class definition
        auto classDef = classRegistry->findClass(typeName);
        if (!classDef)
        {
            return false;
        }

        // Walk up the inheritance chain
        auto chain = getInheritanceChain(type);
        for (const auto& ancestor : chain)
        {
            if (ancestor == baseClassName)
            {
                return true;
            }
        }

        return false;
    }

    bool TypeConstraintValidator::implementsInterface(
        const UnifiedTypePtr& type,
        const std::string& interfaceName) const
    {
        if (!type)
        {
            return false;
        }

        auto implementedInterfaces = getAllImplementedInterfaces(type);
        return implementedInterfaces.count(interfaceName) > 0;
    }

    bool TypeConstraintValidator::isAssignableTo(
        const UnifiedTypePtr& source,
        const UnifiedTypePtr& target) const
    {
        if (!source || !target)
        {
            return false;
        }

        // Same type - always assignable
        if (source->equals(target))
        {
            return true;
        }

        // Null can be assigned to any nullable type
        if (source->isNull() && target->isNullable())
        {
            return true;
        }

        // Null can be assigned to any object/interface type
        if (source->isNull() && (target->isClass() || target->isInterface()))
        {
            return true;
        }

        // Check class inheritance
        if (source->isClass() && target->isClass())
        {
            return extendsClass(source, target->getName());
        }

        // Check interface implementation
        if (source->isClass() && target->isInterface())
        {
            return implementsInterface(source, target->getName());
        }

        // Array covariance
        if (source->isArray() && target->isArray())
        {
            auto sourceElem = source->getElementType();
            auto targetElem = target->getElementType();
            return isAssignableTo(sourceElem, targetElem);
        }

        return false;
    }

    std::unordered_set<std::string> TypeConstraintValidator::getAllImplementedInterfaces(
        const UnifiedTypePtr& type) const
    {
        std::unordered_set<std::string> interfaces;

        if (!type || !classRegistry)
        {
            return interfaces;
        }

        std::string typeName = type->getName();
        auto classDef = classRegistry->findClass(typeName);

        if (classDef)
        {
            collectInterfacesFromClass(classDef, interfaces);
        }

        return interfaces;
    }

    void TypeConstraintValidator::collectInterfacesFromClass(
        const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef,
        std::unordered_set<std::string>& interfaces) const
    {
        if (!classDef)
        {
            return;
        }

        // Collect directly implemented interfaces
        for (const auto& iface : classDef->getImplementedInterfaces())
        {
            // Extract base name (remove generic parameters)
            std::string baseName = extractConstraintTypeName(iface);
            if (!baseName.empty())
            {
                interfaces.insert(baseName);

                // Collect parent interfaces
                std::unordered_set<std::string> visited;
                collectParentInterfaces(baseName, interfaces, visited);
            }
        }

        // Recursively check parent class
        auto parent = classDef->getParentClass();
        if (parent)
        {
            collectInterfacesFromClass(parent, interfaces);
        }
    }

    void TypeConstraintValidator::collectParentInterfaces(
        const std::string& interfaceName,
        std::unordered_set<std::string>& interfaces,
        std::unordered_set<std::string>& visited) const
    {
        if (!interfaceRegistry || visited.count(interfaceName) > 0)
        {
            return;
        }

        visited.insert(interfaceName);

        auto interfaceDef = interfaceRegistry->findInterface(interfaceName);
        if (!interfaceDef)
        {
            return;
        }

        for (const auto& parentName : interfaceDef->getExtendedInterfaces())
        {
            std::string baseName = extractConstraintTypeName(parentName);
            if (!baseName.empty())
            {
                interfaces.insert(baseName);
                collectParentInterfaces(baseName, interfaces, visited);
            }
        }
    }

    std::vector<std::string> TypeConstraintValidator::getInheritanceChain(
        const UnifiedTypePtr& type) const
    {
        std::vector<std::string> chain;

        if (!type || !classRegistry)
        {
            return chain;
        }

        std::string typeName = type->getName();
        auto classDef = classRegistry->findClass(typeName);

        constexpr int MAX_DEPTH = 100;
        int depth = 0;

        while (classDef && depth < MAX_DEPTH)
        {
            chain.push_back(classDef->getName());
            classDef = classDef->getParentClass();
            depth++;
        }

        return chain;
    }

    std::string TypeConstraintValidator::extractConstraintTypeName(
        const std::string& constraint) const
    {
        if (constraint.empty())
        {
            return "";
        }

        // Find the start of generic parameters
        size_t anglePos = constraint.find('<');
        if (anglePos != std::string::npos)
        {
            return constraint.substr(0, anglePos);
        }

        return constraint;
    }
}
