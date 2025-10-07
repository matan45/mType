#include "TypeValidator.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../evaluator/utils/ValueConverter.hpp"
#include <functional>

namespace vm::compiler::types
{
    TypeValidator::TypeValidator(std::shared_ptr<environment::Environment> environment)
        : environment(environment)
    {
    }

    std::string TypeValidator::stripGenericParameters(const std::string& typeName) const
    {
        size_t genericStart = typeName.find('<');
        return (genericStart != std::string::npos) ? typeName.substr(0, genericStart) : typeName;
    }

    bool TypeValidator::checkInterfaceHierarchy(
        const std::string& interfaceName,
        const std::string& targetInterface,
        std::unordered_set<std::string>& visited
    ) const
    {
        // Avoid infinite loops with circular dependencies
        if (visited.count(interfaceName)) {
            return false;
        }
        visited.insert(interfaceName);

        std::string interfaceBaseName = stripGenericParameters(interfaceName);
        std::string targetBaseName = stripGenericParameters(targetInterface);

        // Direct match
        if (interfaceBaseName == targetBaseName) {
            return true;
        }

        // Check extended interfaces recursively
        auto interfaceDef = environment->findInterface(interfaceName);
        if (interfaceDef) {
            const auto& extendedInterfaces = interfaceDef->getExtendedInterfaces();
            for (const auto& extendedInterface : extendedInterfaces) {
                if (checkInterfaceHierarchy(extendedInterface, targetInterface, visited)) {
                    return true;
                }
            }
        }

        return false;
    }

    bool TypeValidator::isClassCompatible(const std::string& derivedClass, const std::string& baseClass) const
    {
        if (derivedClass == baseClass) {
            return true;
        }

        // Check if derivedClass inherits from baseClass
        auto classDef = environment->findClass(derivedClass);
        if (!classDef) {
            return false;
        }

        // Check parent chain
        auto parentClass = classDef->getParentClass();
        while (parentClass) {
            if (parentClass->getName() == baseClass) {
                return true;
            }
            parentClass = parentClass->getParentClass();
        }

        // Check implemented interfaces (with full recursive hierarchy checking)
        const auto& interfaces = classDef->getImplementedInterfaces();
        for (const auto& interfaceName : interfaces) {
            std::unordered_set<std::string> visited;
            if (checkInterfaceHierarchy(interfaceName, baseClass, visited)) {
                return true;
            }
        }

        return false;
    }

    void TypeValidator::validateAssignment(
        value::ValueType varType,
        const std::string& varClassName,
        value::ValueType valueType,
        const std::string& valueClassName,
        bool isNullValue,
        const ast::SourceLocation& location
    ) const
    {
        // null can be assigned to any object type
        if (isNullValue && varType == value::ValueType::OBJECT) {
            return;
        }

        // For OBJECT types, check class compatibility
        if (varType == value::ValueType::OBJECT && valueType == value::ValueType::OBJECT) {
            if (!varClassName.empty() && !valueClassName.empty()) {
                // Extract base class names (remove generic parameters)
                std::string baseVarClass = stripGenericParameters(varClassName);
                std::string baseValueClass = stripGenericParameters(valueClassName);

                // Check class compatibility (including inheritance)
                if (!isClassCompatible(baseValueClass, baseVarClass)) {
                    throw errors::TypeException(
                        "Type mismatch: cannot assign " + valueClassName + " to " + varClassName,
                        location
                    );
                }
            }
            return;
        }

        // For non-OBJECT types, check primitive type compatibility
        if (varType != value::ValueType::OBJECT && valueType != value::ValueType::VOID && valueType != varType) {
            // Special case: INT can be assigned to FLOAT (implicit conversion)
            if (valueType == value::ValueType::INT && varType == value::ValueType::FLOAT) {
                return;
            }

            // Special case: OBJECT can be assigned to STRING (String class to string primitive)
            if (valueType == value::ValueType::OBJECT && varType == value::ValueType::STRING) {
                return;
            }

            std::string varTypeStr = evaluator::utils::ValueConverter::valueTypeToString(varType);
            std::string valueTypeStr = evaluator::utils::ValueConverter::valueTypeToString(valueType);
            throw errors::TypeException(
                "Type mismatch: cannot assign " + valueTypeStr + " to " + varTypeStr,
                location
            );
        }
    }

    void TypeValidator::validateBinaryOperation(
        value::ValueType leftType,
        value::ValueType rightType,
        token::TokenType op,
        bool leftIsNull,
        bool rightIsNull,
        const ast::SourceLocation& location
    ) const
    {
        // Skip validation if we don't know both types
        if (leftType == value::ValueType::VOID || rightType == value::ValueType::VOID) {
            return;
        }

        bool isValid = false;

        // Arithmetic operations: +, -, *, /, %
        if (op == token::TokenType::PLUS || op == token::TokenType::MINUS ||
            op == token::TokenType::MULTIPLY || op == token::TokenType::DIVIDE ||
            op == token::TokenType::MODULO) {

            // String concatenation with +
            if (op == token::TokenType::PLUS &&
                (leftType == value::ValueType::STRING || rightType == value::ValueType::STRING)) {
                isValid = true;
            }
            // Numeric operations: both must be int or float
            else if ((leftType == value::ValueType::INT || leftType == value::ValueType::FLOAT) &&
                     (rightType == value::ValueType::INT || rightType == value::ValueType::FLOAT)) {
                isValid = true;
            }
        }
        // Comparison operations: ==, !=, <, >, <=, >=
        else if (op == token::TokenType::EQUALS || op == token::TokenType::NOT_EQUALS ||
                 op == token::TokenType::LESS || op == token::TokenType::GREATER ||
                 op == token::TokenType::LESS_EQUALS || op == token::TokenType::GREATER_EQUALS) {
            // For == and !=, allow comparing any type with null
            if (op == token::TokenType::EQUALS || op == token::TokenType::NOT_EQUALS) {
                if (leftIsNull || rightIsNull) {
                    isValid = true;
                } else if (leftType == rightType) {
                    isValid = true;
                } else if ((leftType == value::ValueType::INT || leftType == value::ValueType::FLOAT) &&
                          (rightType == value::ValueType::INT || rightType == value::ValueType::FLOAT)) {
                    isValid = true;
                }
            } else {
                // For <, >, <=, >=: only same types or numeric types
                if (leftType == rightType) {
                    isValid = true;
                } else if ((leftType == value::ValueType::INT || leftType == value::ValueType::FLOAT) &&
                          (rightType == value::ValueType::INT || rightType == value::ValueType::FLOAT)) {
                    isValid = true;
                }
            }
        }
        // Logical operations: &&, ||
        else if (op == token::TokenType::AND || op == token::TokenType::OR) {
            if (leftType == value::ValueType::BOOL && rightType == value::ValueType::BOOL) {
                isValid = true;
            }
        }
        else {
            // Unknown operator, allow it
            isValid = true;
        }

        if (!isValid) {
            std::string leftTypeStr = evaluator::utils::ValueConverter::valueTypeToString(leftType);
            std::string rightTypeStr = evaluator::utils::ValueConverter::valueTypeToString(rightType);
            std::string opStr;
            switch (op) {
                case token::TokenType::PLUS: opStr = "+"; break;
                case token::TokenType::MINUS: opStr = "-"; break;
                case token::TokenType::MULTIPLY: opStr = "*"; break;
                case token::TokenType::DIVIDE: opStr = "/"; break;
                case token::TokenType::MODULO: opStr = "%"; break;
                case token::TokenType::AND: opStr = "&&"; break;
                case token::TokenType::OR: opStr = "||"; break;
                default: opStr = "operator"; break;
            }
            throw errors::TypeException(
                "Invalid operation: cannot apply '" + opStr + "' to " + leftTypeStr + " and " + rightTypeStr,
                location
            );
        }
    }
}
