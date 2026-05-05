#include "TypeSubstitutionService.hpp"
#include <cstddef>
#include "../ast/GenericTypeParameter.hpp"
#include <stdexcept>
#include <sstream>

namespace types
{
    UnifiedTypePtr TypeSubstitutionService::substitute(
        const UnifiedTypePtr& type,
        const TypeSubstitutionMap& substitutions) const
    {
        if (!type || substitutions.empty())
        {
            return type;
        }

        // Delegate to UnifiedType's substitute method which has cycle detection
        return type->substitute(substitutions);
    }

    TypeSubstitutionMap TypeSubstitutionService::buildSubstitutionMap(
        const std::vector<ast::GenericTypeParameter>& parameters,
        const std::vector<UnifiedTypePtr>& arguments) const
    {
        if (parameters.size() != arguments.size())
        {
            std::ostringstream oss;
            oss << "Type argument count mismatch: expected " << parameters.size()
                << " but got " << arguments.size();
            throw std::invalid_argument(oss.str());
        }

        TypeSubstitutionMap result;
        for (size_t i = 0; i < parameters.size(); ++i)
        {
            result[parameters[i].name] = arguments[i];
        }

        return result;
    }

    TypeSubstitutionMap TypeSubstitutionService::buildSubstitutionMap(
        const std::vector<std::string>& parameterNames,
        const std::vector<UnifiedTypePtr>& arguments) const
    {
        if (parameterNames.size() != arguments.size())
        {
            std::ostringstream oss;
            oss << "Type argument count mismatch: expected " << parameterNames.size()
                << " but got " << arguments.size();
            throw std::invalid_argument(oss.str());
        }

        TypeSubstitutionMap result;
        for (size_t i = 0; i < parameterNames.size(); ++i)
        {
            result[parameterNames[i]] = arguments[i];
        }

        return result;
    }

    TypeSubstitutionMap TypeSubstitutionService::composeSubstitutions(
        const TypeSubstitutionMap& outer,
        const TypeSubstitutionMap& inner) const
    {
        if (outer.empty())
        {
            return inner;
        }

        if (inner.empty())
        {
            return outer;
        }

        TypeSubstitutionMap result;

        // First, apply inner substitutions to all outer values
        for (const auto& [param, type] : outer)
        {
            if (type)
            {
                result[param] = substitute(type, inner);
            }
            else
            {
                result[param] = type;
            }
        }

        // Then add any inner substitutions not already present
        for (const auto& [param, type] : inner)
        {
            if (result.find(param) == result.end())
            {
                result[param] = type;
            }
        }

        return result;
    }

    TypeSubstitutionMap TypeSubstitutionService::composeChain(
        const std::vector<TypeSubstitutionMap>& chain) const
    {
        if (chain.empty())
        {
            return {};
        }

        if (chain.size() == 1)
        {
            return chain[0];
        }

        TypeSubstitutionMap result = chain[0];
        for (size_t i = 1; i < chain.size(); ++i)
        {
            result = composeSubstitutions(result, chain[i]);
        }

        return result;
    }

    TypeSubstitutionMap TypeSubstitutionService::inferFromArguments(
        const std::vector<UnifiedTypePtr>& parameterTypes,
        const std::vector<UnifiedTypePtr>& argumentTypes,
        const std::unordered_set<std::string>& typeParamNames) const
    {
        TypeSubstitutionMap inferred;
        std::unordered_set<std::string> conflicts;

        size_t count = std::min(parameterTypes.size(), argumentTypes.size());
        for (size_t i = 0; i < count; ++i)
        {
            inferFromSingleArgument(
                parameterTypes[i],
                argumentTypes[i],
                typeParamNames,
                inferred,
                conflicts);
        }

        if (!conflicts.empty())
        {
            std::ostringstream oss;
            oss << "Conflicting type inferences for: ";
            bool first = true;
            for (const auto& param : conflicts)
            {
                if (!first) oss << ", ";
                first = false;
                oss << param;
            }
            throw std::runtime_error(oss.str());
        }

        return inferred;
    }

    void TypeSubstitutionService::inferFromSingleArgument(
        const UnifiedTypePtr& patternType,
        const UnifiedTypePtr& concreteType,
        const std::unordered_set<std::string>& typeParamNames,
        TypeSubstitutionMap& inferred,
        std::unordered_set<std::string>& conflicts) const
    {
        if (!patternType || !concreteType)
        {
            return;
        }

        // If pattern is a type parameter we're looking for, record the inference
        if (patternType->isGenericParameter())
        {
            const std::string& paramName = patternType->getName();
            if (typeParamNames.count(paramName) > 0)
            {
                auto it = inferred.find(paramName);
                if (it == inferred.end())
                {
                    // First inference for this parameter
                    inferred[paramName] = concreteType;
                }
                else
                {
                    // Check for conflicting inference
                    if (!it->second->equals(concreteType))
                    {
                        conflicts.insert(paramName);
                    }
                }
            }
            return;
        }

        // For parameterized types, recursively infer from type arguments
        if (patternType->isParameterized() && concreteType->isParameterized())
        {
            // Types must have same base name
            if (patternType->getName() != concreteType->getName())
            {
                return;
            }

            const auto& patternArgs = patternType->getTypeArguments();
            const auto& concreteArgs = concreteType->getTypeArguments();

            if (patternArgs.size() != concreteArgs.size())
            {
                return;
            }

            for (size_t i = 0; i < patternArgs.size(); ++i)
            {
                inferFromSingleArgument(
                    patternArgs[i],
                    concreteArgs[i],
                    typeParamNames,
                    inferred,
                    conflicts);
            }
        }

        // For arrays, infer from element type
        if (patternType->isArray() && concreteType->isArray())
        {
            inferFromSingleArgument(
                patternType->getElementType(),
                concreteType->getElementType(),
                typeParamNames,
                inferred,
                conflicts);
        }
    }

    std::unordered_set<std::string> TypeSubstitutionService::extractGenericParameters(
        const UnifiedTypePtr& type) const
    {
        std::unordered_set<std::string> params;
        collectGenericParameters(type, params);
        return params;
    }

    void TypeSubstitutionService::collectGenericParameters(
        const UnifiedTypePtr& type,
        std::unordered_set<std::string>& params) const
    {
        if (!type)
        {
            return;
        }

        if (type->isGenericParameter())
        {
            params.insert(type->getName());
            return;
        }

        for (const auto& arg : type->getTypeArguments())
        {
            collectGenericParameters(arg, params);
        }
    }

    bool TypeSubstitutionService::validateSubstitutions(
        const TypeSubstitutionMap& substitutions) const
    {
        for (const auto& [param, type] : substitutions)
        {
            if (!type)
            {
                return false;
            }

            // Check that the substitution doesn't contain unresolved parameters
            // that would create circular dependencies
            if (type->isGenericParameter() && type->getName() == param)
            {
                return false;  // Self-referential substitution
            }
        }

        return true;
    }

    UnifiedTypePtr TypeSubstitutionService::resolveInInheritanceContext(
        const UnifiedTypePtr& type,
        const std::vector<TypeSubstitutionMap>& inheritanceChain) const
    {
        if (!type || inheritanceChain.empty())
        {
            return type;
        }

        // Compose the entire inheritance chain into a single substitution map
        TypeSubstitutionMap composedSubstitutions = composeChain(inheritanceChain);

        // Apply the composed substitutions
        return substitute(type, composedSubstitutions);
    }

    // ============== String-based helper methods ==============

    std::pair<std::string, std::vector<std::string>> TypeSubstitutionService::parseGenericTypeName(
        const std::string& typeName) const
    {
        std::string baseName = extractBaseTypeName(typeName);
        std::vector<std::string> typeArgs = extractTypeArguments(typeName);
        return {baseName, typeArgs};
    }

    std::string TypeSubstitutionService::resolveGenericType(
        const std::string& typeName,
        const std::unordered_map<std::string, std::string>& substitutions) const
    {
        // Strip-recurse-reapply for the nullable suffix (e.g., "T?", "ArrayList<T>?", "T[]?")
        if (!typeName.empty() && typeName.back() == '?')
        {
            std::string baseType = typeName.substr(0, typeName.size() - 1);
            return resolveGenericType(baseType, substitutions) + "?";
        }

        // Check if this is an array type (e.g., "T[]", "T[][]")
        size_t arrayPos = typeName.find('[');
        if (arrayPos != std::string::npos)
        {
            // Extract element type and array dimensions
            std::string elementType = typeName.substr(0, arrayPos);
            std::string arrayDimensions = typeName.substr(arrayPos);

            // Recursively resolve the element type
            std::string resolvedElementType = resolveGenericType(elementType, substitutions);

            // Reconstruct array type with resolved element type
            return resolvedElementType + arrayDimensions;
        }

        // Check if this is a generic parameter that needs substitution
        auto it = substitutions.find(typeName);
        if (it != substitutions.end())
        {
            return it->second;
        }

        // If the type has generic parameters, recursively resolve them
        size_t genericStart = typeName.find('<');
        if (genericStart == std::string::npos)
        {
            return typeName;  // No generic parameters, return as-is
        }

        // Extract base name and type arguments
        std::string baseName = typeName.substr(0, genericStart);
        std::vector<std::string> typeArgs = extractTypeArguments(typeName);

        // Resolve each type argument
        std::vector<std::string> resolvedArgs;
        for (const auto& arg : typeArgs)
        {
            resolvedArgs.push_back(resolveGenericType(arg, substitutions));
        }

        // Reconstruct the type name with resolved arguments
        std::string result = baseName + "<";
        for (size_t i = 0; i < resolvedArgs.size(); ++i)
        {
            if (i > 0) result += ",";
            result += resolvedArgs[i];
        }
        result += ">";

        return result;
    }

    std::string TypeSubstitutionService::extractBaseTypeName(const std::string& typeName) const
    {
        size_t genericStart = typeName.find('<');
        if (genericStart != std::string::npos)
        {
            return typeName.substr(0, genericStart);
        }
        return typeName;
    }

    std::vector<std::string> TypeSubstitutionService::extractTypeArguments(const std::string& typeName) const
    {
        std::vector<std::string> typeArguments;

        size_t genericStart = typeName.find('<');
        if (genericStart == std::string::npos)
        {
            return typeArguments;  // No generic parameters
        }

        size_t genericEnd = typeName.rfind('>');
        if (genericEnd == std::string::npos || genericEnd <= genericStart)
        {
            return typeArguments;  // Invalid format
        }

        std::string typeArgsStr = typeName.substr(genericStart + 1, genericEnd - genericStart - 1);

        // Parse individual type arguments, respecting nested generics
        size_t start = 0;
        size_t depth = 0;
        for (size_t i = 0; i < typeArgsStr.length(); ++i)
        {
            if (typeArgsStr[i] == '<')
            {
                depth++;
            }
            else if (typeArgsStr[i] == '>')
            {
                depth--;
            }
            else if (typeArgsStr[i] == ',' && depth == 0)
            {
                std::string arg = typeArgsStr.substr(start, i - start);
                // Trim whitespace
                arg.erase(0, arg.find_first_not_of(" \t"));
                arg.erase(arg.find_last_not_of(" \t") + 1);
                typeArguments.push_back(arg);
                start = i + 1;
            }
        }

        // Add last argument
        std::string arg = typeArgsStr.substr(start);
        arg.erase(0, arg.find_first_not_of(" \t"));
        arg.erase(arg.find_last_not_of(" \t") + 1);
        if (!arg.empty())
        {
            typeArguments.push_back(arg);
        }

        return typeArguments;
    }
}
