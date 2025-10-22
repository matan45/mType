#include "TypeConversionUtils.hpp"
#include <algorithm>
#include <sstream>
#include <set>

namespace types {

    // Private helper function implementations
    value::ValueType TypeConversionUtils::handleGenericParameter(
        std::shared_ptr<ast::GenericType> genericType,
        const std::unordered_map<std::string, std::string>& substitutionMap,
        TypeRegistry& registry) {

        std::string typeName = genericType->getGenericName();

        auto it = substitutionMap.find(typeName);
        if (it != substitutionMap.end()) {
            std::string substitutedType = it->second;

            // Validate the substituted type
            if (!registry.hasType(substitutedType)) {
                auto suggestions = getTypeSuggestions(substitutedType);
                std::string message = "Substituted type '" + substitutedType + "' is not registered";
                if (!suggestions.empty()) {
                    message += ". Did you mean: " + suggestions[0] + "?";
                }
                throw errors::TypeConversionException(message, substitutedType, "unknown");
            }

            if (registry.isArrayType(substitutedType)) {
                return value::ValueType::ARRAY;
            }

            return registry.getValueType(substitutedType);
        }

        // Generic parameter without substitution - treat as OBJECT
        return value::ValueType::OBJECT;
    }

    value::ValueType TypeConversionUtils::handleConcreteType(
        std::shared_ptr<ast::GenericType> genericType,
        TypeRegistry& registry) {

        try {
            return genericType->getConcreteType();
        } catch (...) {
            std::string typeName = genericType->getBaseTypeName();

            if (!registry.hasType(typeName)) {
                auto suggestions = getTypeSuggestions(typeName);
                std::string message = "Type '" + typeName + "' is not registered";
                if (!suggestions.empty()) {
                    message += ". Did you mean: " + suggestions[0] + "?";
                }
                throw errors::TypeConversionException(message, typeName, "unknown");
            }

            return registry.getValueType(typeName);
        }
    }

    value::ValueType TypeConversionUtils::handleParameterizedType(
        std::shared_ptr<ast::GenericType> genericType,
        TypeRegistry& registry) {

        std::string baseTypeName = genericType->getBaseTypeName();

        if (baseTypeName == "Array" || registry.isArrayType(baseTypeName)) {
            return value::ValueType::ARRAY;
        }

        if (registry.isCollectionType(baseTypeName)) {
            // Validate type arguments
            auto typeArgs = genericType->getTypeArguments();
            std::vector<std::string> typeArgStrings;
            typeArgStrings.reserve(typeArgs.size());
            for (const auto& arg : typeArgs) {
                typeArgStrings.push_back(arg->toString());
            }

            std::string errorMessage;
            if (!validateGenericInstantiation(baseTypeName, typeArgStrings, errorMessage)) {
                throw errors::TypeConversionException(errorMessage, baseTypeName, "object");
            }

            return value::ValueType::OBJECT;
        }

        return value::ValueType::OBJECT;
    }

    // Public API implementation
    value::ValueType TypeConversionUtils::convertWithContext(
        std::shared_ptr<ast::GenericType> genericType,
        const std::unordered_map<std::string, std::string>& substitutionMap,
        const TypeConversionContext& context) {

        if (!genericType) {
            throw errors::TypeConversionException("Cannot convert null generic type", "null", "unknown");
        }

        auto& registry = getGlobalTypeRegistry();

        try {
            // Handle generic parameters with substitution
            if (genericType->isGenericParameter()) {
                return handleGenericParameter(genericType, substitutionMap, registry);
            }

            // Handle concrete types
            if (!genericType->isGenericParameter()) {
                return handleConcreteType(genericType, registry);
            }

            // Handle parameterized types
            if (genericType->isParameterized()) {
                return handleParameterizedType(genericType, registry);
            }

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
        constexpr double SIMILARITY_THRESHOLD = 0.3;  // Minimum similarity to be considered relevant
        constexpr size_t MAX_SUGGESTIONS = 5;         // Maximum number of suggestions to return

        auto& registry = getGlobalTypeRegistry();
        auto allTypes = registry.getAllTypeNames();

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

} // namespace types