#pragma once

#include "../value/ValueType.hpp"
#include "../ast/GenericType.hpp"
#include "TypeRegistry.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/TypeConversionException.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace types {

    /**
     * Context information for type conversion operations
     */
    struct TypeConversionContext {
        std::string sourceLocation;
        std::string operation;
        std::vector<std::string> typeHints;

        TypeConversionContext(const std::string& location = "", const std::string& op = "")
            : sourceLocation(location), operation(op) {}
    };

    /**
     * Enhanced type conversion exception with detailed context
     * @deprecated Use errors::TypeConversionException instead
     */
    class TypeConversionException : public errors::TypeConversionException {
    private:
        TypeConversionContext context_;
        std::vector<std::string> suggestions_;

    public:
        TypeConversionException(const std::string& message,
                              const std::string& sourceType,
                              const std::string& targetType,
                              const TypeConversionContext& context = TypeConversionContext())
            : errors::TypeConversionException(message, sourceType, targetType)
            , context_(context) {}

        const TypeConversionContext& getContext() const { return context_; }
        const std::vector<std::string>& getSuggestions() const { return suggestions_; }

        void addSuggestion(const std::string& suggestion) {
            suggestions_.push_back(suggestion);
        }
    };

    /**
     * Utility class for improved type conversion with comprehensive error handling
     */
    class TypeConversionUtils {
    public:
        /**
         * Convert generic type to ValueType with comprehensive error handling
         */
        static value::ValueType convertWithContext(
            std::shared_ptr<ast::GenericType> genericType,
            const std::unordered_map<std::string, std::string>& substitutionMap,
            const TypeConversionContext& context = TypeConversionContext());

        /**
         * Validate type compatibility between two types
         */
        static bool areTypesCompatible(value::ValueType sourceType, value::ValueType targetType);

        /**
         * Check if a type can be implicitly converted to another
         */
        static bool canImplicitlyConvert(value::ValueType from, value::ValueType to);

        /**
         * Get type suggestions for unknown type names
         */
        static std::vector<std::string> getTypeSuggestions(const std::string& unknownType);

        /**
         * Validate array type dimensions and element types
         */
        static bool validateArrayType(const std::string& typeName,
                                    std::string& elementType,
                                    int& dimensions);

        /**
         * Validate generic type instantiation
         */
        static bool validateGenericInstantiation(const std::string& genericType,
                                               const std::vector<std::string>& typeArguments,
                                               std::string& errorMessage);

        /**
         * Get human-readable type name for ValueType
         */
        static std::string getTypeDisplayName(value::ValueType type);

        /**
         * Check if a string represents a valid type name
         */
        static bool isValidTypeName(const std::string& typeName);

    private:
        /**
         * Calculate string similarity for type suggestions
         */
        static double calculateStringSimilarity(const std::string& str1, const std::string& str2);

        /**
         * Find similar type names in the registry
         */
        static std::vector<std::string> findSimilarTypes(const std::string& typeName);
    };

} // namespace types