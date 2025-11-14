#pragma once
#include "../../../environment/Environment.hpp"
#include "../../../value/ValueType.hpp"
#include "../../../errors/SourceLocation.hpp"
#include "../../../token/TokenType.hpp"
#include <string>
#include <memory>
#include <unordered_set>

namespace vm::compiler::types
{
    /**
     * Validates type compatibility and conversions
     * Checks class inheritance, interface implementation, and type assignments
     */
    class TypeValidator
    {
    public:
        explicit TypeValidator(std::shared_ptr<environment::Environment> environment);
        ~TypeValidator() = default;

        // Class compatibility checking
        bool isClassCompatible(const std::string& derivedClass, const std::string& baseClass) const;

        // Type assignment validation
        void validateAssignment(
            value::ValueType varType,
            const std::string& varClassName,
            value::ValueType valueType,
            const std::string& valueClassName,
            bool isNullValue,
            const ast::SourceLocation& location
        ) const;

        // Binary operation validation
        void validateBinaryOperation(
            value::ValueType leftType,
            value::ValueType rightType,
            token::TokenType op,
            bool leftIsNull,
            bool rightIsNull,
            const ast::SourceLocation& location
        ) const;

        // Binary operation validation with class names (for object comparison checking)
        void validateBinaryOperation(
            value::ValueType leftType,
            const std::string& leftClassName,
            value::ValueType rightType,
            const std::string& rightClassName,
            token::TokenType op,
            bool leftIsNull,
            bool rightIsNull,
            const ast::SourceLocation& location
        ) const;

        // Iterator type validation
        std::string validateAndExtractIterableElementType(
            const std::string& className,
            const std::string& loopVarType,
            const ast::SourceLocation& location
        ) const;

    private:
        std::shared_ptr<environment::Environment> environment;

        // Helper methods
        bool checkInterfaceHierarchy(
            const std::string& interfaceName,
            const std::string& targetInterface,
            std::unordered_set<std::string>& visited
        ) const;

        std::string stripGenericParameters(const std::string& typeName) const;

        // Helper for finding Iterable in interface hierarchy
        std::string findIterableInInterfaceHierarchy(
            const std::string& interfaceName,
            std::unordered_set<std::string>& visited
        ) const;

        // Helper methods for validateAssignment
        std::string normalizeArrayType(const std::string& type) const;
        bool validatePromiseTypeAssignment(const std::string& varClassName, const std::string& valueClassName) const;
        void validateObjectTypeAssignment(const std::string& varClassName, const std::string& valueClassName,
                                          const ast::SourceLocation& location) const;
        void validatePrimitiveToObjectAssignment(value::ValueType varType, const std::string& varClassName,
                                                 value::ValueType valueType, const ast::SourceLocation& location) const;
        void validatePrimitiveTypeAssignment(value::ValueType varType, value::ValueType valueType,
                                             const ast::SourceLocation& location) const;

        // Helper methods for validateBinaryOperation
        bool isArithmeticOperationValid(value::ValueType leftType, value::ValueType rightType, token::TokenType op) const;
        bool isComparisonOperationValid(value::ValueType leftType, value::ValueType rightType, token::TokenType op,
                                       bool leftIsNull, bool rightIsNull) const;
        bool isComparisonOperationValid(value::ValueType leftType, const std::string& leftClassName,
                                       value::ValueType rightType, const std::string& rightClassName,
                                       token::TokenType op, bool leftIsNull, bool rightIsNull) const;
        bool isLogicalOperationValid(value::ValueType leftType, value::ValueType rightType) const;
        void throwBinaryOperationError(value::ValueType leftType, value::ValueType rightType, token::TokenType op,
                                       const ast::SourceLocation& location) const;
    };
}
