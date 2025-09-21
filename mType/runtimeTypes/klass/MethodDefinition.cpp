#include "MethodDefinition.hpp"
#include "../../parser/TypeParser.hpp"
#include <algorithm>

namespace runtimeTypes::klass
{
    ValueType MethodDefinition::resolveParameterType(size_t paramIndex) const
    {
        if (paramIndex >= parameters.size())
        {
            return ValueType::VOID;
        }

        // If we have generic information and a substitution map, try to resolve the type
        if (hasGenericInformation() && !typeSubstitutionMap.empty())
        {
            // Get the stored parameter type
            ValueType storedType = parameters[paramIndex].second;

            // If the stored type is OBJECT, it might represent a generic type parameter
            // For generic methods, OBJECT parameters should be substituted with concrete types

            if (storedType == ValueType::OBJECT)
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

                // Fallback: Use position-based mapping for OBJECT parameters
                // But we need to be smart about when to resolve vs when to keep as OBJECT

                // Check if this parameter name suggests it's a collection instance
                std::string paramName = parameters[paramIndex].first;
                bool isLikelyCollectionParam =
                    paramName.find("other") != std::string::npos ||
                    paramName.find("set") != std::string::npos ||
                    paramName.find("list") != std::string::npos ||
                    paramName.find("map") != std::string::npos ||
                    paramName.find("queue") != std::string::npos ||
                    paramName.find("stack") != std::string::npos;

                // If it looks like a collection parameter, keep it as OBJECT
                if (isLikelyCollectionParam)
                {
                    return ValueType::OBJECT;
                }

                // For other OBJECT parameters, try position-based resolution
                // Count how many OBJECT parameters we've seen before this one
                size_t objectParamIndex = 0;
                for (size_t j = 0; j < paramIndex; ++j)
                {
                    if (parameters[j].second == ValueType::OBJECT)
                    {
                        // Skip collection-like parameters in the count
                        std::string otherParamName = parameters[j].first;
                        bool isOtherCollectionParam =
                            otherParamName.find("other") != std::string::npos ||
                            otherParamName.find("set") != std::string::npos ||
                            otherParamName.find("list") != std::string::npos ||
                            otherParamName.find("map") != std::string::npos ||
                            otherParamName.find("queue") != std::string::npos ||
                            otherParamName.find("stack") != std::string::npos;

                        if (!isOtherCollectionParam)
                        {
                            objectParamIndex++;
                        }
                    }
                }

                // Map OBJECT parameters to type parameters in alphabetical order
                std::vector<std::string> sortedTypeParams;
                for (const auto& [key, value] : typeSubstitutionMap)
                {
                    sortedTypeParams.push_back(key);
                }
                std::sort(sortedTypeParams.begin(), sortedTypeParams.end());

                // Use the nth type parameter for the nth non-collection OBJECT parameter
                if (objectParamIndex < sortedTypeParams.size())
                {
                    auto it = typeSubstitutionMap.find(sortedTypeParams[objectParamIndex]);
                    if (it != typeSubstitutionMap.end())
                    {
                        return parser::TypeParser::stringToValueType(it->second);
                    }
                }
            }
        }

        // Fall back to the stored ValueType
        return parameters[paramIndex].second;
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
        if (hasGenericInformation() && returnType == ValueType::OBJECT)
        {
            return ValueType::OBJECT;
        }

        // If we have generic information and a substitution map, resolve the return type
        if (hasGenericInformation() && genericReturnType && genericReturnType->isGenericParameter())
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
}
