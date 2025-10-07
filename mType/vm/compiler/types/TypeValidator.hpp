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

    private:
        std::shared_ptr<environment::Environment> environment;

        // Helper methods
        bool checkInterfaceHierarchy(
            const std::string& interfaceName,
            const std::string& targetInterface,
            std::unordered_set<std::string>& visited
        ) const;

        std::string stripGenericParameters(const std::string& typeName) const;
    };
}
