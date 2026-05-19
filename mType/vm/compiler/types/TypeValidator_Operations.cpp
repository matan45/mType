#include "TypeValidator.hpp"
#include <cstddef>
#include <unordered_set>
#include "../../../errors/TypeException.hpp"
#include "../../../types/TypeConversionUtils.hpp"

namespace vm::compiler::types
{
    bool TypeValidator::isArithmeticOperationValid(value::ValueType leftType, value::ValueType rightType, token::TokenType op) const
    {
        // String concatenation with +
        if (op == token::TokenType::PLUS &&
            (leftType == value::ValueType::STRING || rightType == value::ValueType::STRING)) {
            return true;
        }

        // Numeric operations: both must be int or float
        if ((leftType == value::ValueType::INT || leftType == value::ValueType::FLOAT) &&
            (rightType == value::ValueType::INT || rightType == value::ValueType::FLOAT)) {
            return true;
        }

        // Allow operations on wrapper object types (Int, Float, String, Bool)
        // These will be translated to method calls (add, subtract, multiply, divide, etc.)
        if (leftType == value::ValueType::OBJECT && rightType == value::ValueType::OBJECT) {
            return true;
        }

        // Allow mixed primitive and object types (auto-boxing/unboxing)
        if ((leftType == value::ValueType::OBJECT &&
             (rightType == value::ValueType::INT || rightType == value::ValueType::FLOAT ||
              rightType == value::ValueType::STRING || rightType == value::ValueType::BOOL)) ||
            (rightType == value::ValueType::OBJECT &&
             (leftType == value::ValueType::INT || leftType == value::ValueType::FLOAT ||
              leftType == value::ValueType::STRING || leftType == value::ValueType::BOOL))) {
            return true;
        }

        return false;
    }

    bool TypeValidator::isComparisonOperationValid(value::ValueType leftType, value::ValueType rightType, token::TokenType op,
                                                   bool leftIsNull, bool rightIsNull) const
    {
        // For == and !=, allow comparing any type with null
        if (op == token::TokenType::EQUALS || op == token::TokenType::NOT_EQUALS) {
            if (leftIsNull || rightIsNull) {
                return true;
            }
            if (leftType == rightType) {
                return true;
            }
            // Allow comparing numeric types
            if ((leftType == value::ValueType::INT || leftType == value::ValueType::FLOAT) &&
                (rightType == value::ValueType::INT || rightType == value::ValueType::FLOAT)) {
                return true;
            }
        } else {
            // For <, >, <=, >=: only same types or numeric types
            if (leftType == rightType) {
                return true;
            }
            if ((leftType == value::ValueType::INT || leftType == value::ValueType::FLOAT) &&
                (rightType == value::ValueType::INT || rightType == value::ValueType::FLOAT)) {
                return true;
            }
        }

        return false;
    }

    bool TypeValidator::isComparisonOperationValid(
        value::ValueType leftType,
        const std::string& leftClassName,
        value::ValueType rightType,
        const std::string& rightClassName,
        token::TokenType op,
        bool leftIsNull,
        bool rightIsNull
    ) const
    {
        // For == and !=, allow comparing any type with null
        if (op == token::TokenType::EQUALS || op == token::TokenType::NOT_EQUALS) {
            if (leftIsNull || rightIsNull) {
                return true;
            }

            // Allow comparing numeric primitives
            if ((leftType == value::ValueType::INT || leftType == value::ValueType::FLOAT) &&
                (rightType == value::ValueType::INT || rightType == value::ValueType::FLOAT)) {
                return true;
            }

            // For objects, check class names
            if (leftType == value::ValueType::OBJECT && rightType == value::ValueType::OBJECT) {
                // Disallow comparing unrelated object types, but allow boxed primitives
                // (Int, Float, Bool, String) for auto-boxing.
                static const std::unordered_set<std::string> boxedPrimitives = {"Int", "Float", "Bool", "String"};

                bool leftIsBoxed = boxedPrimitives.find(leftClassName) != boxedPrimitives.end();
                bool rightIsBoxed = boxedPrimitives.find(rightClassName) != boxedPrimitives.end();

                if (leftClassName == rightClassName) {
                    return true;
                }
                if (leftIsBoxed && rightIsBoxed) {
                    return true;
                }

                return false;
            }

            // For same primitive types
            if (leftType == rightType && leftType != value::ValueType::OBJECT) {
                return true;
            }

        } else {
            // For <, >, <=, >=: only same types or numeric types
            if ((leftType == value::ValueType::INT || leftType == value::ValueType::FLOAT) &&
                (rightType == value::ValueType::INT || rightType == value::ValueType::FLOAT)) {
                return true;
            }

            // For objects, only allow if same class
            if (leftType == value::ValueType::OBJECT && rightType == value::ValueType::OBJECT) {
                return leftClassName == rightClassName;
            }

            // For same primitive types
            if (leftType == rightType && leftType != value::ValueType::OBJECT) {
                return true;
            }
        }

        return false;
    }

    bool TypeValidator::isLogicalOperationValid(value::ValueType leftType, value::ValueType rightType) const
    {
        // PHASE 4: Allow Bool objects (which will be auto-unboxed) or primitive bools.
        // Logical operations work with: bool && bool, Bool && Bool, bool && Bool, Bool && bool
        if ((leftType == value::ValueType::BOOL || leftType == value::ValueType::OBJECT) &&
            (rightType == value::ValueType::BOOL || rightType == value::ValueType::OBJECT)) {
            return true;
        }
        return false;
    }

    bool TypeValidator::isBitwiseOperationValid(value::ValueType leftType, value::ValueType rightType) const
    {
        return leftType == value::ValueType::INT && rightType == value::ValueType::INT;
    }

    void TypeValidator::throwBinaryOperationError(value::ValueType leftType, value::ValueType rightType, token::TokenType op,
                                                  const ast::SourceLocation& location) const
    {
        std::string leftTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(leftType);
        std::string rightTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(rightType);
        std::string opStr;

        switch (op) {
            case token::TokenType::PLUS: opStr = "+"; break;
            case token::TokenType::MINUS: opStr = "-"; break;
            case token::TokenType::MULTIPLY: opStr = "*"; break;
            case token::TokenType::DIVIDE: opStr = "/"; break;
            case token::TokenType::MODULO: opStr = "%"; break;
            case token::TokenType::AND: opStr = "&&"; break;
            case token::TokenType::OR: opStr = "||"; break;
            case token::TokenType::BITWISE_AND: opStr = "&"; break;
            case token::TokenType::BITWISE_OR: opStr = "|"; break;
            case token::TokenType::BITWISE_XOR: opStr = "^"; break;
            case token::TokenType::LEFT_SHIFT: opStr = "<<"; break;
            case token::TokenType::RIGHT_SHIFT: opStr = ">>"; break;
            default: opStr = "operator"; break;
        }

        throw errors::TypeException(
            "Invalid operation: cannot apply '" + opStr + "' to " + leftTypeStr + " and " + rightTypeStr,
            location
        );
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
        if (leftType == value::ValueType::VOID || rightType == value::ValueType::VOID) {
            return;
        }

        bool isValid = false;

        if (op == token::TokenType::PLUS || op == token::TokenType::MINUS ||
            op == token::TokenType::MULTIPLY || op == token::TokenType::DIVIDE ||
            op == token::TokenType::MODULO) {
            isValid = isArithmeticOperationValid(leftType, rightType, op);
        }
        else if (op == token::TokenType::EQUALS || op == token::TokenType::NOT_EQUALS ||
                 op == token::TokenType::LESS || op == token::TokenType::GREATER ||
                 op == token::TokenType::LESS_EQUALS || op == token::TokenType::GREATER_EQUALS) {
            isValid = isComparisonOperationValid(leftType, rightType, op, leftIsNull, rightIsNull);
        }
        else if (op == token::TokenType::AND || op == token::TokenType::OR) {
            isValid = isLogicalOperationValid(leftType, rightType);
        }
        else if (op == token::TokenType::BITWISE_AND || op == token::TokenType::BITWISE_OR ||
                 op == token::TokenType::BITWISE_XOR || op == token::TokenType::LEFT_SHIFT ||
                 op == token::TokenType::RIGHT_SHIFT) {
            isValid = isBitwiseOperationValid(leftType, rightType);
        }
        else {
            isValid = true;
        }

        if (!isValid) {
            throwBinaryOperationError(leftType, rightType, op, location);
        }
    }

    void TypeValidator::validateBinaryOperation(
        value::ValueType leftType,
        const std::string& leftClassName,
        value::ValueType rightType,
        const std::string& rightClassName,
        token::TokenType op,
        bool leftIsNull,
        bool rightIsNull,
        const ast::SourceLocation& location
    ) const
    {
        if (leftType == value::ValueType::VOID || rightType == value::ValueType::VOID) {
            return;
        }

        bool isValid = false;

        if (op == token::TokenType::PLUS || op == token::TokenType::MINUS ||
            op == token::TokenType::MULTIPLY || op == token::TokenType::DIVIDE ||
            op == token::TokenType::MODULO) {
            isValid = isArithmeticOperationValid(leftType, rightType, op);
        }
        else if (op == token::TokenType::EQUALS || op == token::TokenType::NOT_EQUALS ||
                 op == token::TokenType::LESS || op == token::TokenType::GREATER ||
                 op == token::TokenType::LESS_EQUALS || op == token::TokenType::GREATER_EQUALS) {
            isValid = isComparisonOperationValid(leftType, leftClassName, rightType, rightClassName, op, leftIsNull, rightIsNull);
        }
        else if (op == token::TokenType::AND || op == token::TokenType::OR) {
            isValid = isLogicalOperationValid(leftType, rightType);
        }
        else if (op == token::TokenType::BITWISE_AND || op == token::TokenType::BITWISE_OR ||
                 op == token::TokenType::BITWISE_XOR || op == token::TokenType::LEFT_SHIFT ||
                 op == token::TokenType::RIGHT_SHIFT) {
            isValid = isBitwiseOperationValid(leftType, rightType);
        }
        else {
            isValid = true;
        }

        if (!isValid) {
            throwBinaryOperationError(leftType, rightType, op, location);
        }
    }

    std::string TypeValidator::findIterableInInterfaceHierarchy(
        const std::string& interfaceName,
        std::unordered_set<std::string>& visited
    ) const
    {
        std::string baseInterfaceName = stripGenericParameters(interfaceName);
        if (visited.count(baseInterfaceName)) {
            return "";
        }
        visited.insert(baseInterfaceName);

        if (baseInterfaceName == "Iterable") {
            // "FOUND" marker — caller extracts element type from the collection class.
            return "FOUND";
        }

        auto interfaceDef = environment->findInterface(baseInterfaceName);
        if (interfaceDef) {
            const auto& extendedInterfaces = interfaceDef->getExtendedInterfaces();
            for (const auto& extendedInterface : extendedInterfaces) {
                std::string result = findIterableInInterfaceHierarchy(extendedInterface, visited);
                if (!result.empty()) {
                    return result;
                }
            }
        }

        return "";
    }

    std::string TypeValidator::validateAndExtractIterableElementType(
        const std::string& className,
        const std::string& loopVarType,
        const ast::SourceLocation& location
    ) const
    {
        std::string baseClassName = stripGenericParameters(className);

        auto classDef = environment->findClass(baseClassName);
        if (!classDef) {
            throw errors::TypeException(
                "Type error: Cannot use for-each loop on type '" + className +
                "' - class definition not found",
                location
            );
        }

        // Check if the class implements Iterable<T> (directly or through interface hierarchy)
        const auto& interfaces = classDef->getImplementedInterfaces();
        bool implementsIterable = false;

        for (const auto& interfaceName : interfaces) {
            std::unordered_set<std::string> visited;
            std::string result = findIterableInInterfaceHierarchy(interfaceName, visited);
            if (!result.empty()) {
                implementsIterable = true;
                break;
            }
        }

        // If not found in direct interfaces, check parent class
        if (!implementsIterable) {
            std::string parentClassName = classDef->getParentClassName();
            if (!parentClassName.empty()) {
                auto parentClass = environment->findClass(parentClassName);
                if (parentClass) {
                    const auto& parentInterfaces = parentClass->getImplementedInterfaces();
                    for (const auto& interfaceName : parentInterfaces) {
                        std::unordered_set<std::string> visited;
                        std::string result = findIterableInInterfaceHierarchy(interfaceName, visited);
                        if (!result.empty()) {
                            implementsIterable = true;
                            break;
                        }
                    }
                }
            }
        }

        if (!implementsIterable) {
            throw errors::TypeException(
                "Type error: Cannot use for-each loop on type '" + className +
                "' - it does not implement Iterable<T> interface",
                location
            );
        }

        // Extract element type from the collection's generic parameters.
        // ArrayList<String> -> "String"; HashMap<K,V> iterates over keys -> "K" (first parameter).
        std::string elementType;
        size_t openBracket = className.find('<');
        size_t closeBracket = className.rfind('>');

        if (openBracket != std::string::npos && closeBracket != std::string::npos &&
            closeBracket > openBracket) {
            std::string genericParams = className.substr(openBracket + 1, closeBracket - openBracket - 1);

            size_t commaPos = genericParams.find(',');
            if (commaPos != std::string::npos) {
                elementType = genericParams.substr(0, commaPos);
                size_t firstNonSpace = elementType.find_first_not_of(" \t");
                size_t lastNonSpace = elementType.find_last_not_of(" \t");
                if (firstNonSpace != std::string::npos && lastNonSpace != std::string::npos) {
                    elementType = elementType.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);
                }
            } else {
                elementType = genericParams;
            }
        } else {
            elementType = "Object";
        }

        std::string strippedLoopVarType = stripGenericParameters(loopVarType);
        std::string strippedElementType = stripGenericParameters(elementType);

        if (strippedLoopVarType != strippedElementType) {
            if (!isClassCompatible(strippedElementType, strippedLoopVarType)) {
                throw errors::TypeException(
                    "Type error: For-each loop variable type '" + loopVarType +
                    "' is incompatible with iterator element type '" + elementType +
                    "'. Collection '" + className + "' is Iterable<" + elementType + ">",
                    location
                );
            }
        }

        return elementType;
    }
}
