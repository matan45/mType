#include "TypeConversionUtils.hpp"
#include <algorithm>
#include <sstream>
#include <set>

namespace types {

    // createDetailedMessage method removed - functionality moved to base class

    value::ValueType TypeConversionUtils::convertWithContext(
        std::shared_ptr<ast::GenericType> genericType,
        const std::unordered_map<std::string, std::string>& substitutionMap,
        const TypeConversionContext& context) {

        if (!genericType) {
            throw TypeConversionException("null", "Cannot convert null generic type", context);
        }

        auto& registry = getGlobalTypeRegistry();

        try {
            // Handle generic parameters with substitution
            if (genericType->isGenericParameter()) {
                std::string typeName = genericType->getGenericName();

                auto it = substitutionMap.find(typeName);
                if (it != substitutionMap.end()) {
                    std::string substitutedType = it->second;

                    // Validate the substituted type
                    if (!registry.hasType(substitutedType)) {
                        TypeConversionException ex(substitutedType,
                            "Substituted type is not registered", context);

                        auto suggestions = getTypeSuggestions(substitutedType);
                        for (const auto& suggestion : suggestions) {
                            ex.addSuggestion(suggestion);
                        }
                        throw ex;
                    }

                    if (registry.isArrayType(substitutedType)) {
                        return value::ValueType::ARRAY;
                    }

                    return registry.getValueType(substitutedType);
                }

                // Generic parameter without substitution - treat as OBJECT
                return value::ValueType::OBJECT;
            }

            // Handle concrete types
            if (!genericType->isGenericParameter()) {
                try {
                    return genericType->getConcreteType();
                } catch (...) {
                    std::string typeName = genericType->getBaseTypeName();

                    if (!registry.hasType(typeName)) {
                        TypeConversionException ex(typeName,
                            "Type is not registered", context);

                        auto suggestions = getTypeSuggestions(typeName);
                        for (const auto& suggestion : suggestions) {
                            ex.addSuggestion(suggestion);
                        }
                        throw ex;
                    }

                    return registry.getValueType(typeName);
                }
            }

            // Handle parameterized types
            if (genericType->isParameterized()) {
                std::string baseTypeName = genericType->getBaseTypeName();

                if (baseTypeName == "Array" || registry.isArrayType(baseTypeName)) {
                    return value::ValueType::ARRAY;
                }

                if (registry.isCollectionType(baseTypeName)) {
                    // Validate type arguments
                    auto typeArgs = genericType->getTypeArguments();
                    std::vector<std::string> typeArgStrings;
                    for (const auto& arg : typeArgs) {
                        typeArgStrings.push_back(arg->toString());
                    }

                    std::string errorMessage;
                    if (!validateGenericInstantiation(baseTypeName, typeArgStrings, errorMessage)) {
                        throw TypeConversionException(baseTypeName, errorMessage, context);
                    }

                    return value::ValueType::OBJECT;
                }

                return value::ValueType::OBJECT;
            }

            return value::ValueType::OBJECT;

        } catch (const TypeConversionException&) {
            throw; // Re-throw our detailed exceptions
        } catch (const std::exception& e) {
            throw TypeConversionException("unknown",
                std::string("Unexpected error: ") + e.what(), context);
        }
    }

    bool TypeConversionUtils::areTypesCompatible(value::ValueType sourceType, value::ValueType targetType) {
        // Exact match
        if (sourceType == targetType) {
            return true;
        }

        // Null can be assigned to any object or array type
        if (sourceType == value::ValueType::NULL_TYPE) {
            return targetType == value::ValueType::OBJECT ||
                   targetType == value::ValueType::ARRAY;
        }

        // Arrays and objects are compatible in some contexts
        if ((sourceType == value::ValueType::ARRAY && targetType == value::ValueType::OBJECT) ||
            (sourceType == value::ValueType::OBJECT && targetType == value::ValueType::ARRAY)) {
            return true;
        }

        return false;
    }

    bool TypeConversionUtils::canImplicitlyConvert(value::ValueType from, value::ValueType to) {
        // Check for compatible types
        if (areTypesCompatible(from, to)) {
            return true;
        }

        // Numeric conversions
        if (from == value::ValueType::INT && to == value::ValueType::FLOAT) {
            return true;
        }

        return false;
    }

    std::vector<std::string> TypeConversionUtils::getTypeSuggestions(const std::string& unknownType) {
        auto& registry = getGlobalTypeRegistry();
        auto allTypes = registry.getAllTypeNames();

        std::vector<std::pair<std::string, double>> scoredSuggestions;

        for (const auto& typeName : allTypes) {
            double similarity = calculateStringSimilarity(unknownType, typeName);
            if (similarity > 0.3) { // Threshold for relevance
                scoredSuggestions.emplace_back(typeName, similarity);
            }
        }

        // Sort by similarity score (descending)
        std::sort(scoredSuggestions.begin(), scoredSuggestions.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        // Return top 5 suggestions
        std::vector<std::string> suggestions;
        for (size_t i = 0; i < std::min(size_t(5), scoredSuggestions.size()); ++i) {
            suggestions.push_back(scoredSuggestions[i].first);
        }

        return suggestions;
    }

    bool TypeConversionUtils::validateArrayType(const std::string& typeName,
                                              std::string& elementType,
                                              int& dimensions) {
        auto [parsedElementType, parsedDimensions] = TypeRegistry::parseArrayTypeName(typeName);

        if (parsedDimensions <= 0) {
            return false;
        }

        auto& registry = getGlobalTypeRegistry();
        if (!registry.hasType(parsedElementType)) {
            return false;
        }

        elementType = parsedElementType;
        dimensions = parsedDimensions;
        return true;
    }

    bool TypeConversionUtils::validateGenericInstantiation(const std::string& genericType,
                                                          const std::vector<std::string>& typeArguments,
                                                          std::string& errorMessage) {
        auto& registry = getGlobalTypeRegistry();

        if (!registry.isGenericType(genericType)) {
            errorMessage = "'" + genericType + "' is not a generic type";
            return false;
        }

        auto expectedParams = registry.getGenericTypeParameters(genericType);
        if (typeArguments.size() != expectedParams.size()) {
            errorMessage = "Expected " + std::to_string(expectedParams.size()) +
                          " type arguments, got " + std::to_string(typeArguments.size());
            return false;
        }

        // Validate each type argument
        for (size_t i = 0; i < typeArguments.size(); ++i) {
            const auto& typeArg = typeArguments[i];
            if (!registry.hasType(typeArg) && !TypeRegistry::isGenericParameter(typeArg)) {
                errorMessage = "Type argument '" + typeArg + "' is not a valid type";
                return false;
            }
        }

        return true;
    }

    std::string TypeConversionUtils::getTypeDisplayName(value::ValueType type) {
        switch (type) {
            case value::ValueType::INT: return "int";
            case value::ValueType::FLOAT: return "float";
            case value::ValueType::BOOL: return "bool";
            case value::ValueType::STRING: return "string";
            case value::ValueType::VOID: return "void";
            case value::ValueType::OBJECT: return "object";
            case value::ValueType::ARRAY: return "array";
            case value::ValueType::LAMBDA: return "lambda";
            case value::ValueType::NULL_TYPE: return "null";
            default: return "unknown";
        }
    }

    bool TypeConversionUtils::isValidTypeName(const std::string& typeName) {
        if (typeName.empty()) {
            return false;
        }

        // Check basic naming rules
        if (!std::isalpha(typeName[0]) && typeName[0] != '_') {
            return false;
        }

        for (char c : typeName) {
            if (!std::isalnum(c) && c != '_' && c != '<' && c != '>' &&
                c != '[' && c != ']' && c != ',' && c != ':' && c != ' ') {
                return false;
            }
        }

        return true;
    }

    double TypeConversionUtils::calculateStringSimilarity(const std::string& str1, const std::string& str2) {
        // Simple Levenshtein distance-based similarity
        size_t len1 = str1.length();
        size_t len2 = str2.length();

        if (len1 == 0) return len2 == 0 ? 1.0 : 0.0;
        if (len2 == 0) return 0.0;

        std::vector<std::vector<size_t>> dp(len1 + 1, std::vector<size_t>(len2 + 1));

        for (size_t i = 0; i <= len1; ++i) dp[i][0] = i;
        for (size_t j = 0; j <= len2; ++j) dp[0][j] = j;

        for (size_t i = 1; i <= len1; ++i) {
            for (size_t j = 1; j <= len2; ++j) {
                if (str1[i-1] == str2[j-1]) {
                    dp[i][j] = dp[i-1][j-1];
                } else {
                    dp[i][j] = 1 + std::min({dp[i-1][j], dp[i][j-1], dp[i-1][j-1]});
                }
            }
        }

        size_t maxLen = std::max(len1, len2);
        return 1.0 - (double)dp[len1][len2] / maxLen;
    }

    std::vector<std::string> TypeConversionUtils::findSimilarTypes(const std::string& typeName) {
        return getTypeSuggestions(typeName);
    }

} // namespace types