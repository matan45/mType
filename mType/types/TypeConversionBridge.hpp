#pragma once

#include "UnifiedType.hpp"
#include "../ast/GenericType.hpp"
#include "../ast/GenericTypeParameter.hpp"
#include <memory>
#include <vector>
#include <string>

namespace types
{
    /**
     * Bridge utilities for converting between the old (GenericType) and new (UnifiedType) type systems.
     *
     * This class provides conversion functions during the migration period.
     * Once migration is complete, these can be removed.
     */
    class TypeConversionBridge
    {
    public:
        // ============== GenericType <-> UnifiedType ==============

        /**
         * Convert ast::GenericType to types::UnifiedType.
         */
        static UnifiedTypePtr toUnifiedType(const std::shared_ptr<ast::GenericType>& genericType);

        /**
         * Convert types::UnifiedType to ast::GenericType.
         */
        static std::shared_ptr<ast::GenericType> toGenericType(const UnifiedTypePtr& unifiedType);

        // ============== GenericTypeParameter Constraint Conversion ==============

        /**
         * Convert ast::GenericTypeParameter constraints (strings) to types::TypeConstraint.
         *
         * @param param The generic type parameter with string constraints
         * @return Vector of TypeConstraint objects
         */
        static std::vector<TypeConstraint> convertConstraints(
            const ast::GenericTypeParameter& param);

        /**
         * Convert a single constraint string to TypeConstraint.
         *
         * @param constraintStr The constraint string (e.g., "Comparable", "Comparable<T>")
         * @param kind The constraint kind (Extends or Implements)
         * @return TypeConstraint object
         */
        static TypeConstraint parseConstraintString(
            const std::string& constraintStr,
            TypeConstraint::Kind kind = TypeConstraint::Kind::Implements);

        /**
         * Convert GenericTypeParameter to UnifiedType (as a generic parameter with constraints).
         */
        static UnifiedTypePtr toUnifiedTypeParam(const ast::GenericTypeParameter& param);

        // ============== Batch Conversions ==============

        /**
         * Convert a vector of GenericTypeParameters to UnifiedType generic parameters.
         */
        static std::vector<UnifiedTypePtr> convertGenericParams(
            const std::vector<ast::GenericTypeParameter>& params);

        /**
         * Extract parameter names from GenericTypeParameter vector.
         */
        static std::vector<std::string> extractParamNames(
            const std::vector<ast::GenericTypeParameter>& params);

    private:
        /**
         * Parse constraint type name, handling nested generics.
         * e.g., "Comparable<T>" -> UnifiedType for Comparable<T>
         */
        static UnifiedTypePtr parseConstraintType(const std::string& constraintStr);

        /**
         * Extract base type name from constraint (strips generic args).
         */
        static std::string extractBaseTypeName(const std::string& typeName);
    };
}
