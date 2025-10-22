#include "MethodDefinition.hpp"
#include "../../parser/TypeParser.hpp"
#include "../../ast/nodes/expressions/LambdaInterfaceInvocationNode.hpp"
#include <algorithm>
#include <sstream>

namespace runtimeTypes::klass
{
    ValueType MethodDefinition::resolveGenericParameter(size_t paramIndex, ValueType storedType) const
    {
        // Check if we have generic parameter information for this parameter
        if (paramIndex < genericParameters.size())
        {
            auto genericParam = genericParameters[paramIndex].second;
            if (genericParam)
            {
                if (genericParam->isGenericParameter())
                {
                    // This is a direct generic type parameter (T, K, V) - resolve it
                    std::string typeParamName = genericParam->getGenericName();
                    auto it = typeSubstitutionMap.find(typeParamName);
                    if (it != typeSubstitutionMap.end())
                    {
                        return parser::TypeParser::stringToValueType(it->second);
                    }
                }
                else if (genericParam->isParameterized())
                {
                    // This is a generic collection/object type (e.g., Set<T>, List<U>) - keep as OBJECT
                    return ValueType::OBJECT;
                }
            }
        }
        return storedType;
    }

    ValueType MethodDefinition::resolveFallbackMapping(size_t paramIndex) const
    {
        // Count how many OBJECT parameters we've seen before this one
        size_t objectParamIndex = 0;
        for (size_t j = 0; j < paramIndex; ++j)
        {
            if (parameters[j].second == ValueType::OBJECT)
            {
                objectParamIndex++;
            }
        }

        // Map OBJECT parameters to type parameters in alphabetical order
        std::vector<std::string> sortedTypeParams;
        for (const auto& [key, value] : typeSubstitutionMap)
        {
            sortedTypeParams.push_back(key);
        }
        std::sort(sortedTypeParams.begin(), sortedTypeParams.end());

        // Use the nth type parameter for the nth OBJECT parameter
        if (objectParamIndex < sortedTypeParams.size())
        {
            auto it = typeSubstitutionMap.find(sortedTypeParams[objectParamIndex]);
            if (it != typeSubstitutionMap.end())
            {
                // With boxed types, we only resolve if the target type is also OBJECT
                // If target is primitive, keep as OBJECT (since boxed types are objects)
                value::ValueType targetType = parser::TypeParser::stringToValueType(it->second);
                if (targetType == ValueType::OBJECT)
                {
                    return targetType;
                }
                else
                {
                    // Target is primitive, but parameter is boxed object - keep as OBJECT
                    return ValueType::OBJECT;
                }
            }
        }
        return ValueType::OBJECT;
    }

    ValueType MethodDefinition::resolveParameterType(size_t paramIndex) const
    {
        if (paramIndex >= parameters.size())
        {
            return ValueType::VOID;
        }

        // Get the stored parameter type
        ValueType storedType = parameters[paramIndex].second;

        // If we have generic information and a substitution map, try to resolve the type
        if (isGeneric() && !typeSubstitutionMap.empty() && storedType == ValueType::OBJECT)
        {
            // Try to resolve using generic parameter information
            ValueType resolved = resolveGenericParameter(paramIndex, storedType);
            if (resolved != storedType)
            {
                return resolved;
            }

            // Fallback: Use position-based mapping for OBJECT parameters
            return resolveFallbackMapping(paramIndex);
        }

        // Fall back to the stored ValueType
        return storedType;
    }

    ValueType MethodDefinition::resolveParameterType(const std::string& paramName) const
    {
        // Find parameter by name
        for (size_t i = 0; i < parameters.size(); ++i)
        {
            if (parameters[i].first == paramName)
            {
                return resolveParameterType(i);
            }
        }
        return ValueType::VOID;
    }

    ValueType MethodDefinition::resolveReturnType() const
    {
        // For methods that return generic collections like Set<T>, the return type should be OBJECT
        if (isGeneric() && returnType == ValueType::OBJECT)
        {
            return ValueType::OBJECT;
        }

        // If we have generic information and a substitution map, resolve the return type
        if (isGeneric() && genericReturnType && genericReturnType->isGenericParameter())
        {
            std::string typeName = genericReturnType->getGenericName();
            auto it = typeSubstitutionMap.find(typeName);
            if (it != typeSubstitutionMap.end())
            {
                return parser::TypeParser::stringToValueType(it->second);
            }
        }

        // Fall back to the stored ValueType
        return returnType;
    }

    void MethodDefinition::cleanupLambdaResources()
    {
        // Reset lambda implementation
        lambdaImplementation.reset();

        // Clear weak reference to lambda node
        lambdaNode.reset();

        // If the body is a LambdaInterfaceInvocationNode, clean it up
        if (body) {
            if (auto lambdaInvocationNode = std::dynamic_pointer_cast<ast::nodes::expressions::LambdaInterfaceInvocationNode>(body)) {
                lambdaInvocationNode->cleanup();
            }
        }
    }

    bool MethodDefinition::needsLambdaCleanup() const
    {
        // Check if we have lambda resources that need cleanup
        if (hasLambdaImplementation() || hasLambdaNode()) {
            // Check if lambda node is expired
            if (!isLambdaNodeValid()) {
                return true;
            }

            // Check if lambda invocation node needs cleanup
            if (body) {
                if (auto lambdaInvocationNode = std::dynamic_pointer_cast<const ast::nodes::expressions::LambdaInterfaceInvocationNode>(body)) {
                    return lambdaInvocationNode->needsCleanup();
                }
            }
        }

        return false;
    }

    std::string MethodDefinition::getLambdaLifecycleStatus() const
    {
        std::stringstream ss;
        ss << "MethodDefinition(name=" << getName();

        if (hasLambdaImplementation()) {
            ss << ", hasLambdaImpl=true";
        }

        if (hasLambdaNode()) {
            ss << ", hasLambdaNode=true, nodeValid=" << (isLambdaNodeValid() ? "true" : "false");
        }

        if (body) {
            if (auto lambdaInvocationNode = std::dynamic_pointer_cast<const ast::nodes::expressions::LambdaInterfaceInvocationNode>(body)) {
                ss << ", invocationNode=" << lambdaInvocationNode->getLambdaLifecycleStatus();
            }
        }

        ss << ")";
        return ss.str();
    }
}
