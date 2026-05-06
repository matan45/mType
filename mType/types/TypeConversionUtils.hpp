#pragma once

#include "../value/ValueType.hpp"
#include "../value/ParameterType.hpp"
#include "UnifiedType.hpp"
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
     * Utility class for improved type conversion with comprehensive error handling
     */
    class TypeConversionUtils {
    public:
        /**
         * Convert generic type to ValueType with comprehensive error handling
         */
        static value::ValueType convertWithContext(
            const UnifiedTypePtr& type,
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

        // ---- getTypeDisplayName overloads ----------------------------------
        //
        // Two overloads, used in different layers: the enum-only form for
        // error-message paths where only a ValueType is in scope, and the
        // ParameterType form (MYT-282) for signature / overload-resolution
        // paths that need precise array names.
        //
        /**
         * Get human-readable type name for ValueType. Returns "array" for
         * ARRAY-tag inputs — element-type information is unavailable here
         * by design; use the ParameterType overload when it's available.
         */
        static std::string getTypeDisplayName(value::ValueType type);

        /**
         * MYT-282: precise display name for a parameter type — preserves
         * `int[]`, `Animal[]`, etc. for ARRAY tags (instead of the coarse
         * "array" string the enum-only overload returns) and falls through
         * to the class/interface name for OBJECT tags. Used for method
         * signatures, overload-resolution keys, and error messages where
         * element type information matters.
         */
        static std::string getTypeDisplayName(const value::ParameterType& paramType);

        /**
         * Check if a string represents a valid type name
         */
        static bool isValidTypeName(const std::string& typeName);

        /**
         * Convert type name string to ValueType enum
         * @param typeName Type name (e.g., "int", "float", "string", "bool", "void")
         * @return Corresponding ValueType enum value
         */
        static value::ValueType stringToValueType(const std::string& typeName);

        /**
         * Extract base class name from generic type (e.g., "Box<Int>" -> "Box")
         * @param typeName Full type name potentially including generics
         * @return Base class name without generic parameters
         */
        static std::string extractBaseTypeName(const std::string& typeName);

        /**
         * Strip the trailing '?' nullable suffix from a type name.
         * Returns the input unchanged if it does not end with '?'.
         * @param typeName Type name potentially ending with '?' (e.g., "Box?")
         * @return Type name without nullable suffix (e.g., "Box")
         */
        static std::string stripNullable(const std::string& typeName);

        /**
         * Check whether a type name ends with the nullable '?' suffix.
         */
        static bool isNullableType(const std::string& typeName);

        /**
         * Check whether a type name represents a generic type parameter
         * (single uppercase letter like T, K, V, E, R).
         */
        static bool isGenericTypeParameter(const std::string& typeName);

    private:
        /**
         * Handle conversion of generic type parameters
         */
        static value::ValueType handleGenericParameter(
            const UnifiedTypePtr& type,
            const std::unordered_map<std::string, std::string>& substitutionMap,
            TypeRegistry& registry);

        /**
         * Handle conversion of concrete types
         */
        static value::ValueType handleConcreteType(
            const UnifiedTypePtr& type,
            TypeRegistry& registry);

        /**
         * Handle conversion of parameterized types
         */
        static value::ValueType handleParameterizedType(
            const UnifiedTypePtr& type,
            TypeRegistry& registry);

        /**
         * Calculate string similarity for type suggestions
         */
        static double calculateStringSimilarity(const std::string& str1, const std::string& str2);
    };

} // namespace types