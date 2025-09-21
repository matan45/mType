#include "MethodDefinition.hpp"
#include "../../parser/TypeParser.hpp"
#include <iostream>
#include <algorithm>

namespace runtimeTypes::klass
{
    ValueType MethodDefinition::resolveParameterType(size_t paramIndex) const
    {
        if (paramIndex >= parameters.size()) {
            return ValueType::VOID;
        }

        // If we have generic information and a substitution map, try to resolve the type
        if (hasGenericInformation() && !typeSubstitutionMap.empty()) {
            // Get the stored parameter type
            ValueType storedType = parameters[paramIndex].second;

            // If the stored type is OBJECT, it might represent a generic type parameter
            // For generic methods, OBJECT parameters should be substituted with concrete types

            if (storedType == ValueType::OBJECT) {
                // For multi-type parameter classes, we need a smarter way to determine
                // which type parameter this OBJECT represents

                // Simple heuristic: if we have substitution map, try to find the right type
                // For now, we'll try each substitution and see if it makes sense
                // This is not perfect but works for common cases

                if (typeSubstitutionMap.size() == 1) {
                    // Single type parameter case - but we need to check if this is a generic class parameter
                    // For Set<T> other, the parameter should remain OBJECT (not resolve to T)
                    // Only primitive type parameters like T item should resolve to the concrete type

                    // Check parameter name to determine if it should stay as object
                    if (paramIndex < parameters.size()) {
                        const std::string& paramName = parameters[paramIndex].first;

                        auto it = typeSubstitutionMap.begin();

                        // If parameter name suggests it's a generic collection/object, keep as OBJECT
                        if (paramName.find("other") != std::string::npos ||
                            paramName.find("collection") != std::string::npos ||
                            paramName.find("set") != std::string::npos ||
                            paramName.find("list") != std::string::npos) {
                            return ValueType::OBJECT;
                        }

                        // Otherwise resolve to concrete type for primitive parameters
                        return parser::TypeParser::stringToValueType(it->second);
                    }
                } else {
                    // Multi-type parameter case - use method name heuristic
                    // For common patterns like setFirst/setSecond, getFirst/getSecond
                    std::string methodName = getName(); // Use the method name
                    std::string paramName = parameters[paramIndex].first;

                    // For Map<K,V> classes, use parameter position and common name patterns
                    if (typeSubstitutionMap.find("K") != typeSubstitutionMap.end() &&
                        typeSubstitutionMap.find("V") != typeSubstitutionMap.end()) {
                        // This is a Map<K,V> class - use parameter position and name heuristics

                        // Check for key-related parameters (K type)
                        if (paramName.find("key") != std::string::npos ||
                            paramName.find("Key") != std::string::npos) {
                            auto it = typeSubstitutionMap.find("K");
                            if (it != typeSubstitutionMap.end()) {
                                return parser::TypeParser::stringToValueType(it->second);
                            }
                        }

                        // Check for value-related parameters (V type)
                        if (paramName.find("value") != std::string::npos ||
                            paramName.find("Value") != std::string::npos) {
                            auto it = typeSubstitutionMap.find("V");
                            if (it != typeSubstitutionMap.end()) {
                                return parser::TypeParser::stringToValueType(it->second);
                            }
                        }

                        // Count OBJECT parameters to determine position-based mapping
                        int objectParamIndex = 0;
                        for (size_t j = 0; j < paramIndex; ++j) {
                            if (parameters[j].second == ValueType::OBJECT) {
                                objectParamIndex++;
                            }
                        }

                        // First OBJECT parameter -> K, second OBJECT parameter -> V
                        if (objectParamIndex == 0) {
                            auto it = typeSubstitutionMap.find("K");
                            if (it != typeSubstitutionMap.end()) {
                                return parser::TypeParser::stringToValueType(it->second);
                            }
                        } else if (objectParamIndex == 1) {
                            auto it = typeSubstitutionMap.find("V");
                            if (it != typeSubstitutionMap.end()) {
                                return parser::TypeParser::stringToValueType(it->second);
                            }
                        }
                    }

                    // Fallback to method name patterns
                    if (methodName.find("First") != std::string::npos) {
                        // Methods with "First" in name use the first type parameter
                        auto it = typeSubstitutionMap.find("T");
                        if (it != typeSubstitutionMap.end()) {
                            return parser::TypeParser::stringToValueType(it->second);
                        }
                    } else if (methodName.find("Second") != std::string::npos) {
                        // Methods with "Second" in name use the second type parameter
                        auto it = typeSubstitutionMap.find("U");
                        if (it != typeSubstitutionMap.end()) {
                            return parser::TypeParser::stringToValueType(it->second);
                        }
                    } else if (methodName.find("Key") != std::string::npos) {
                        // Methods with "Key" in name use the key type parameter
                        auto it = typeSubstitutionMap.find("K");
                        if (it != typeSubstitutionMap.end()) {
                            return parser::TypeParser::stringToValueType(it->second);
                        }
                    } else if (methodName.find("Value") != std::string::npos) {
                        // Methods with "Value" in name use the value type parameter
                        auto it = typeSubstitutionMap.find("V");
                        if (it != typeSubstitutionMap.end()) {
                            return parser::TypeParser::stringToValueType(it->second);
                        }
                    }

                    // Fallback: use alphabetical order of type parameters
                    std::vector<std::string> sortedKeys;
                    for (const auto& [key, value] : typeSubstitutionMap) {
                        sortedKeys.push_back(key);
                    }
                    std::sort(sortedKeys.begin(), sortedKeys.end());

                    // Use first type parameter for first OBJECT parameter
                    if (paramIndex == 0 && !sortedKeys.empty()) {
                        auto it = typeSubstitutionMap.find(sortedKeys[0]);
                        if (it != typeSubstitutionMap.end()) {
                            return parser::TypeParser::stringToValueType(it->second);
                        }
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
        for (size_t i = 0; i < parameters.size(); ++i) {
            if (parameters[i].first == paramName) {
                return resolveParameterType(i);
            }
        }
        return ValueType::VOID;
    }

    ValueType MethodDefinition::resolveReturnType() const
    {
        // For methods that return generic collections like Set<T>, the return type should be OBJECT
        if (hasGenericInformation() && returnType == ValueType::OBJECT) {
            return ValueType::OBJECT;
        }

        // If we have generic information and a substitution map, resolve the return type
        if (hasGenericInformation() && genericReturnType && genericReturnType->isGenericParameter()) {
            std::string typeName = genericReturnType->getGenericName();
            auto it = typeSubstitutionMap.find(typeName);
            if (it != typeSubstitutionMap.end()) {
                return parser::TypeParser::stringToValueType(it->second);
            }
        }

        // Fall back to the stored ValueType
        return returnType;
    }
}