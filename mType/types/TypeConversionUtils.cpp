#include "TypeConversionUtils.hpp"
#include "../environment/registry/TypeCatalog.hpp"
#include <cstddef>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <set>

namespace types {

    // Private helper function implementations
    value::ValueType TypeConversionUtils::handleGenericParameter(
        const UnifiedTypePtr& type,
        const std::unordered_map<std::string, std::string>& substitutionMap,
        environment::registry::TypeCatalog& catalog) {

        std::string typeName = type->getName();

        auto it = substitutionMap.find(typeName);
        if (it != substitutionMap.end()) {
            std::string substitutedType = it->second;

            // Validate the substituted type
            if (!catalog.hasType(substitutedType)) {
                auto suggestions = getTypeSuggestions(substitutedType, catalog);
                std::string message = "Substituted type '" + substitutedType + "' is not registered";
                if (!suggestions.empty()) {
                    message += ". Did you mean: " + suggestions[0] + "?";
                }
                throw errors::TypeConversionException(message, substitutedType, "unknown");
            }

            if (catalog.isArrayType(substitutedType)) {
                return value::ValueType::ARRAY;
            }

            return catalog.getValueType(substitutedType);
        }

        // Generic parameter without substitution - treat as OBJECT
        return value::ValueType::OBJECT;
    }

    value::ValueType TypeConversionUtils::handleConcreteType(
        const UnifiedTypePtr& type,
        environment::registry::TypeCatalog& catalog) {

        try {
            return type->toValueType();
        } catch (...) {
            std::string typeName = type->getName();

            if (!catalog.hasType(typeName)) {
                auto suggestions = getTypeSuggestions(typeName, catalog);
                std::string message = "Type '" + typeName + "' is not registered";
                if (!suggestions.empty()) {
                    message += ". Did you mean: " + suggestions[0] + "?";
                }
                throw errors::TypeConversionException(message, typeName, "unknown");
            }

            return catalog.getValueType(typeName);
        }
    }

    value::ValueType TypeConversionUtils::handleParameterizedType(
        const UnifiedTypePtr& type,
        environment::registry::TypeCatalog& catalog) {

        std::string baseTypeName = type->getName();

        if (baseTypeName == "Array" || catalog.isArrayType(baseTypeName)) {
            return value::ValueType::ARRAY;
        }

        if (catalog.isCollectionType(baseTypeName)) {
            // Validate type arguments
            const auto& typeArgs = type->getTypeArguments();
            std::vector<std::string> typeArgStrings;
            typeArgStrings.reserve(typeArgs.size());
            for (const auto& arg : typeArgs) {
                typeArgStrings.push_back(arg->toString());
            }

            std::string errorMessage;
            if (!validateGenericInstantiation(baseTypeName, typeArgStrings, errorMessage, catalog)) {
                throw errors::TypeConversionException(errorMessage, baseTypeName, "object");
            }

            return value::ValueType::OBJECT;
        }

        return value::ValueType::OBJECT;
    }

    // Public API implementation
    value::ValueType TypeConversionUtils::convertWithContext(
        const UnifiedTypePtr& type,
        const std::unordered_map<std::string, std::string>& substitutionMap,
        environment::registry::TypeCatalog& catalog,
        const TypeConversionContext& context) {

        if (!type) {
            throw errors::TypeConversionException("Cannot convert null type", "null", "unknown");
        }

        try {
            // Handle generic parameters with substitution
            if (type->isGenericParameter()) {
                return handleGenericParameter(type, substitutionMap, catalog);
            }

            // Handle parameterized types
            if (type->isParameterized()) {
                return handleParameterizedType(type, catalog);
            }

            // Handle concrete types
            return handleConcreteType(type, catalog);

            return value::ValueType::OBJECT;

        } catch (const errors::TypeConversionException&) {
            throw; // Re-throw our detailed exceptions
        } catch (const std::exception& e) {
            throw errors::TypeConversionException(std::string("Unexpected error: ") + e.what(),
                "unknown", "unknown");
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

        // MYT-281: arrays widen to Object (assignment-rule subtyping). The
        // reverse direction (Object → Array) is intentionally rejected — an
        // explicit `(T[])obj` cast is required to narrow back. Soundness for
        // covariant array assignment is enforced at the array-store site in
        // ArrayExecutor, matching the Java model the project already adopts
        // (see arrayCovariance.mt).
        if (sourceType == value::ValueType::ARRAY && targetType == value::ValueType::OBJECT) {
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

    std::vector<std::string> TypeConversionUtils::getTypeSuggestions(
        const std::string& unknownType,
        environment::registry::TypeCatalog& catalog) {
        constexpr double SIMILARITY_THRESHOLD = 0.3;  // Minimum similarity to be considered relevant
        constexpr size_t MAX_SUGGESTIONS = 5;         // Maximum number of suggestions to return

        auto allTypes = catalog.getAllTypeNames();

        std::vector<std::pair<std::string, double>> scoredSuggestions;
        scoredSuggestions.reserve(allTypes.size() / 2);  // Reserve space for performance

        for (const auto& typeName : allTypes) {
            double similarity = calculateStringSimilarity(unknownType, typeName);
            if (similarity > SIMILARITY_THRESHOLD) {
                scoredSuggestions.emplace_back(typeName, similarity);
            }
        }

        // Sort by similarity score (descending)
        std::sort(scoredSuggestions.begin(), scoredSuggestions.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        // Return top suggestions
        std::vector<std::string> suggestions;
        const size_t numSuggestions = std::min(MAX_SUGGESTIONS, scoredSuggestions.size());
        suggestions.reserve(numSuggestions);

        for (size_t i = 0; i < numSuggestions; ++i) {
            suggestions.push_back(scoredSuggestions[i].first);
        }

        return suggestions;
    }

    bool TypeConversionUtils::validateArrayType(const std::string& typeName,
                                              std::string& elementType,
                                              int& dimensions,
                                              environment::registry::TypeCatalog& catalog) {
        auto [parsedElementType, parsedDimensions] = ArrayTypeParser::parseArrayTypeName(typeName);

        if (parsedDimensions <= 0) {
            return false;
        }

        if (!catalog.hasType(parsedElementType)) {
            return false;
        }

        elementType = parsedElementType;
        dimensions = parsedDimensions;
        return true;
    }

    bool TypeConversionUtils::validateGenericInstantiation(const std::string& genericType,
                                                          const std::vector<std::string>& typeArguments,
                                                          std::string& errorMessage,
                                                          environment::registry::TypeCatalog& catalog) {
        if (!catalog.isGenericType(genericType)) {
            errorMessage = "'" + genericType + "' is not a generic type";
            return false;
        }

        auto expectedParams = catalog.getGenericTypeParameters(genericType);
        if (typeArguments.size() != expectedParams.size()) {
            errorMessage = "Expected " + std::to_string(expectedParams.size()) +
                          " type arguments, got " + std::to_string(typeArguments.size());
            return false;
        }

        // Validate each type argument
        for (size_t i = 0; i < typeArguments.size(); ++i) {
            const auto& typeArg = typeArguments[i];
            if (!catalog.hasType(typeArg) && !environment::registry::TypeCatalog::isGenericParameter(typeArg)) {
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

    std::string TypeConversionUtils::getTypeDisplayName(const value::ParameterType& paramType) {
        // MYT-282: prefer the precise array form when the parameter carries
        // it; fall back to the class/interface name for OBJECT, and finally
        // to the enum-only overload for everything else.
        if (paramType.basicType == value::ValueType::ARRAY &&
            paramType.arrayElementTypeName.has_value()) {
            return paramType.arrayElementTypeName.value();
        }
        if (paramType.isClass()) {
            return paramType.className.value();
        }
        if (paramType.isInterface()) {
            return paramType.interfaceName.value();
        }
        return getTypeDisplayName(paramType.basicType);
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

    value::ValueType TypeConversionUtils::stringToValueType(const std::string& typeName) {
        if (typeName == "int") return value::ValueType::INT;
        if (typeName == "float") return value::ValueType::FLOAT;
        if (typeName == "bool") return value::ValueType::BOOL;
        if (typeName == "string") return value::ValueType::STRING;
        if (typeName == "void") return value::ValueType::VOID;
        // For any object/class type
        return value::ValueType::OBJECT;
    }

    std::string TypeConversionUtils::extractBaseTypeName(const std::string& typeName) {
        size_t genericStart = typeName.find('<');
        if (genericStart != std::string::npos) {
            return typeName.substr(0, genericStart);
        }
        return typeName;
    }

    double TypeConversionUtils::calculateStringSimilarity(const std::string& str1, const std::string& str2) {
        // Optimized Levenshtein distance using single row (O(min(n,m)) space instead of O(n*m))
        const size_t len1 = str1.length();
        const size_t len2 = str2.length();

        // Handle edge cases
        if (len1 == 0) return len2 == 0 ? 1.0 : 0.0;
        if (len2 == 0) return 0.0;

        // Ensure we use the shorter string for the row to minimize space
        const std::string& shorter = (len1 < len2) ? str1 : str2;
        const std::string& longer = (len1 < len2) ? str2 : str1;
        const size_t shorterLen = std::min(len1, len2);
        const size_t longerLen = std::max(len1, len2);

        // Use single row instead of full matrix for space optimization
        std::vector<size_t> prevRow(shorterLen + 1);
        std::vector<size_t> currRow(shorterLen + 1);

        // Initialize first row
        for (size_t j = 0; j <= shorterLen; ++j) {
            prevRow[j] = j;
        }

        // Calculate edit distance row by row
        for (size_t i = 1; i <= longerLen; ++i) {
            currRow[0] = i;
            for (size_t j = 1; j <= shorterLen; ++j) {
                if (longer[i - 1] == shorter[j - 1]) {
                    currRow[j] = prevRow[j - 1];
                } else {
                    currRow[j] = 1 + std::min({
                        prevRow[j],      // deletion
                        currRow[j - 1],  // insertion
                        prevRow[j - 1]   // substitution
                    });
                }
            }
            std::swap(prevRow, currRow);
        }

        // Calculate similarity as 1 - (distance / maxLength)
        const size_t distance = prevRow[shorterLen];
        return 1.0 - static_cast<double>(distance) / longerLen;
    }

    std::string TypeConversionUtils::stripNullable(const std::string& typeName)
    {
        if (!typeName.empty() && typeName.back() == '?')
        {
            return typeName.substr(0, typeName.size() - 1);
        }
        return typeName;
    }

    bool TypeConversionUtils::isNullableType(const std::string& typeName)
    {
        return !typeName.empty() && typeName.back() == '?';
    }

    bool TypeConversionUtils::isGenericTypeParameter(const std::string& typeName)
    {
        return typeName.length() == 1 && std::isupper(static_cast<unsigned char>(typeName[0]));
    }

    bool TypeConversionUtils::containsGenericTypeParameter(const std::string& typeName)
    {
        std::string token;
        for (char ch : typeName)
        {
            unsigned char c = static_cast<unsigned char>(ch);
            if (std::isalnum(c) || ch == '_')
            {
                token.push_back(ch);
                continue;
            }

            if (!token.empty())
            {
                if (isGenericTypeParameter(token))
                {
                    return true;
                }
                token.clear();
            }
        }

        return !token.empty() && isGenericTypeParameter(token);
    }

} // namespace types
