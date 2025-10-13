#include "TypeValidator.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../runtime/utils/TypeConverter.hpp"
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

        // Normalize array types: convert "int[]" to "Array<int>", "int[][]" to "Array<Array<int>>", etc.
        auto normalizeArrayType = [](const std::string& type) -> std::string {
            std::string normalized = type;
            size_t arrayDepth = 0;

            // Count array brackets from the end
            while (normalized.length() >= 2 && normalized.substr(normalized.length() - 2) == "[]") {
                arrayDepth++;
                normalized = normalized.substr(0, normalized.length() - 2);
            }

            // Wrap in Array<> for each dimension
            for (size_t i = 0; i < arrayDepth; ++i) {
                normalized = "Array<" + normalized + ">";
            }

            return normalized;
        };

        // For OBJECT types, check class compatibility
        if (varType == value::ValueType::OBJECT && valueType == value::ValueType::OBJECT) {
            if (!varClassName.empty() && !valueClassName.empty()) {
                // Skip validation for generic array types (like Array<T>, T[], etc.)
                // When the expected type is a generic array, accept any concrete array type
                if ((varClassName == "Array" || varClassName.find("Array<") == 0) &&
                    valueClassName.find("[]") != std::string::npos) {
                    return; // Any concrete array type is acceptable for generic array variable
                }

                // Special case: Promise<T> can accept T (async functions auto-wrap)
                // This allows: return msg; in async function returning Promise<Message>
                // Also handles arrays: Promise<int[]> can accept int[]
                if (varClassName.find("Promise<") == 0) {
                    // Extract the inner type from Promise<T>
                    size_t start = varClassName.find('<') + 1;
                    size_t end = varClassName.rfind('>');
                    if (start != std::string::npos && end != std::string::npos && end > start) {
                        std::string innerType = varClassName.substr(start, end - start);
                        // Trim whitespace from inner type
                        innerType.erase(0, innerType.find_first_not_of(" \t"));
                        innerType.erase(innerType.find_last_not_of(" \t") + 1);

                        // Direct match: int[] == int[]
                        if (valueClassName == innerType) {
                            return; // T is acceptable for Promise<T> (will be wrapped automatically)
                        }

                        // Match with generic parameters stripped: List<Int> matches List<T>
                        if (stripGenericParameters(valueClassName) == stripGenericParameters(innerType)) {
                            return; // T is acceptable for Promise<T> (will be wrapped automatically)
                        }

                        // Normalize array types for comparison: int[] -> Array<int>
                        std::string normalizedInnerType = normalizeArrayType(innerType);
                        std::string normalizedValueClass = normalizeArrayType(valueClassName);

                        if (normalizedValueClass == normalizedInnerType ||
                            stripGenericParameters(normalizedValueClass) == stripGenericParameters(normalizedInnerType)) {
                            return; // Array type is acceptable for Promise<array> (will be wrapped automatically)
                        }
                    }
                }

                // Normalize both types for array comparison
                std::string normalizedVarClass = normalizeArrayType(varClassName);
                std::string normalizedValueClass = normalizeArrayType(valueClassName);

                // Extract base class names (remove generic parameters)
                std::string baseVarClass = stripGenericParameters(normalizedVarClass);
                std::string baseValueClass = stripGenericParameters(normalizedValueClass);

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

            std::string varTypeStr = vm::runtime::utils::TypeConverter::valueTypeToString(varType);
            std::string valueTypeStr = vm::runtime::utils::TypeConverter::valueTypeToString(valueType);
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
            std::string leftTypeStr = vm::runtime::utils::TypeConverter::valueTypeToString(leftType);
            std::string rightTypeStr = vm::runtime::utils::TypeConverter::valueTypeToString(rightType);
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
