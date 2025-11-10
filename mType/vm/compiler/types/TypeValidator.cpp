#include "TypeValidator.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../types/TypeConversionUtils.hpp"
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

        // Check if derivedClass is a class
        auto classDef = environment->findClass(derivedClass);
        if (classDef) {
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

        // Check if derivedClass is an interface that extends baseClass
        auto interfaceDef = environment->findInterface(derivedClass);
        if (interfaceDef) {
            std::unordered_set<std::string> visited;
            if (checkInterfaceHierarchy(derivedClass, baseClass, visited)) {
                return true;
            }
        }

        return false;
    }

    std::string TypeValidator::normalizeArrayType(const std::string& type) const
    {
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
    }

    bool TypeValidator::validatePromiseTypeAssignment(const std::string& varClassName, const std::string& valueClassName) const
    {
        // Special case: Promise<T> can accept T (async functions auto-wrap)
        if (varClassName.find("Promise<") != 0) {
            return false;
        }

        // Extract the inner type from Promise<T>
        size_t start = varClassName.find('<') + 1;
        size_t end = varClassName.rfind('>');
        if (start == std::string::npos || end == std::string::npos || end <= start) {
            return false;
        }

        std::string innerType = varClassName.substr(start, end - start);
        // Trim whitespace from inner type
        innerType.erase(0, innerType.find_first_not_of(" \t"));
        innerType.erase(innerType.find_last_not_of(" \t") + 1);

        // Direct match: int[] == int[]
        if (valueClassName == innerType) {
            return true;
        }

        // Match with generic parameters stripped: List<Int> matches List<T>
        if (stripGenericParameters(valueClassName) == stripGenericParameters(innerType)) {
            return true;
        }

        // Normalize array types for comparison: int[] -> Array<int>
        std::string normalizedInnerType = normalizeArrayType(innerType);
        std::string normalizedValueClass = normalizeArrayType(valueClassName);

        return (normalizedValueClass == normalizedInnerType ||
                stripGenericParameters(normalizedValueClass) == stripGenericParameters(normalizedInnerType));
    }

    void TypeValidator::validateObjectTypeAssignment(const std::string& varClassName, const std::string& valueClassName,
                                                     const ast::SourceLocation& location) const
    {
        // Skip validation for generic array types
        if ((varClassName == "Array" || varClassName.find("Array<") == 0) &&
            valueClassName.find("[]") != std::string::npos) {
            return;
        }

        // Check Promise type special handling
        if (validatePromiseTypeAssignment(varClassName, valueClassName)) {
            return;
        }

        // Array covariance check: arrays must have EXACT element type match
        // (cannot assign Dog[] to Animal[] even though Dog extends Animal)
        bool varIsArray = (varClassName.find("[]") != std::string::npos);
        bool valueIsArray = (valueClassName.find("[]") != std::string::npos);

        if (varIsArray && valueIsArray) {
            // Extract element types from both arrays (e.g., "Animal" from "Animal[]")
            std::string varElementType = varClassName.substr(0, varClassName.find("[]"));
            std::string valueElementType = valueClassName.substr(0, valueClassName.find("[]"));

            // Require exact match for array element types (no inheritance/polymorphism)
            if (varElementType != valueElementType) {
                throw errors::TypeException(
                    "Array covariance violation: cannot assign " + valueElementType + "[] to " +
                    varElementType + "[] (arrays are invariant, not covariant)",
                    location
                );
            }
            return; // Arrays match, no further checking needed
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

    void TypeValidator::validatePrimitiveToObjectAssignment(value::ValueType varType, const std::string& varClassName,
                                                            value::ValueType valueType, const ast::SourceLocation& location) const
    {
        if (varType != value::ValueType::OBJECT || valueType == value::ValueType::OBJECT || valueType == value::ValueType::VOID) {
            return;
        }

        // PHASE 4: Allow primitive-to-Box-type assignments (auto-boxing)
        if ((varClassName == "Int" && valueType == value::ValueType::INT) ||
            (varClassName == "Float" && valueType == value::ValueType::FLOAT) ||
            (varClassName == "Bool" && valueType == value::ValueType::BOOL) ||
            (varClassName == "String" && valueType == value::ValueType::STRING)) {
            return; // Auto-boxing will handle this
        }

        // Reject primitive values when OBJECT with specific class is expected
        if (!varClassName.empty()) {
            std::string valueTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(valueType);
            throw errors::TypeException(
                "Type mismatch: cannot assign primitive " + valueTypeStr + " to object type " + varClassName,
                location
            );
        }
    }

    void TypeValidator::validatePrimitiveTypeAssignment(value::ValueType varType, value::ValueType valueType,
                                                        const ast::SourceLocation& location) const
    {
        if (varType == value::ValueType::OBJECT || valueType == value::ValueType::VOID || valueType == varType) {
            return;
        }

        // Special case: INT can be assigned to FLOAT (implicit conversion)
        if (valueType == value::ValueType::INT && varType == value::ValueType::FLOAT) {
            return;
        }

        // Special case: OBJECT can be assigned to STRING (String class to string primitive)
        if (valueType == value::ValueType::OBJECT && varType == value::ValueType::STRING) {
            return;
        }

        std::string varTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(varType);
        std::string valueTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(valueType);
        throw errors::TypeException(
            "Type mismatch: cannot assign " + valueTypeStr + " to " + varTypeStr,
            location
        );
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

        // Special handling for array assignments
        // Arrays can be ARRAY type or OBJECT type depending on context
        bool varIsArray = (varType == value::ValueType::ARRAY) ||
                         (varType == value::ValueType::OBJECT && !varClassName.empty() &&
                          (varClassName.find("[]") != std::string::npos || varClassName.find("Array<") == 0));
        bool valueIsArray = (valueType == value::ValueType::ARRAY) ||
                           (valueType == value::ValueType::OBJECT && !valueClassName.empty() &&
                            (valueClassName.find("[]") != std::string::npos || valueClassName.find("Array<") == 0));

        if (varIsArray && valueIsArray) {
            // Both are arrays - validate array type compatibility
            if (!varClassName.empty() && !valueClassName.empty()) {
                validateObjectTypeAssignment(varClassName, valueClassName, location);
            }
            return;
        }

        // For OBJECT types, check class compatibility
        if (varType == value::ValueType::OBJECT && valueType == value::ValueType::OBJECT) {
            if (!varClassName.empty() && !valueClassName.empty()) {
                validateObjectTypeAssignment(varClassName, valueClassName, location);
            }
            return;
        }

        // Check if trying to assign a primitive to an OBJECT type
        validatePrimitiveToObjectAssignment(varType, varClassName, valueType, location);

        // PHASE 4: Allow Box object to primitive assignment (auto-unboxing)
        if (varType != value::ValueType::OBJECT && valueType == value::ValueType::OBJECT)
        {
            // Check if value is a Box type that can be auto-unboxed
            if ((varType == value::ValueType::INT && valueClassName == "Int") ||
                (varType == value::ValueType::FLOAT && valueClassName == "Float") ||
                (varType == value::ValueType::BOOL && valueClassName == "Bool") ||
                (varType == value::ValueType::STRING && valueClassName == "String"))
            {
                return;  // Auto-unboxing will handle this
            }
        }

        // For non-OBJECT types, check primitive type compatibility
        validatePrimitiveTypeAssignment(varType, valueType, location);
    }

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
                // Disallow comparing unrelated object types
                // EXCEPTION: Allow boxed primitives (Int, Float, Bool, String) for auto-boxing
                static const std::unordered_set<std::string> boxedPrimitives = {"Int", "Float", "Bool", "String"};

                bool leftIsBoxed = boxedPrimitives.find(leftClassName) != boxedPrimitives.end();
                bool rightIsBoxed = boxedPrimitives.find(rightClassName) != boxedPrimitives.end();

                // Allow comparison if:
                // 1. Both are the same class
                // 2. Both are boxed primitives (for auto-boxing)
                if (leftClassName == rightClassName) {
                    return true;
                }
                if (leftIsBoxed && rightIsBoxed) {
                    return true;  // Allow comparing different boxed primitives for auto-boxing
                }

                // Otherwise, disallow comparing unrelated object types
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
        // PHASE 4: Allow Bool objects (which will be auto-unboxed) or primitive bools
        // Logical operations work with: bool && bool, Bool && Bool, bool && Bool, Bool && bool
        if ((leftType == value::ValueType::BOOL || leftType == value::ValueType::OBJECT) &&
            (rightType == value::ValueType::BOOL || rightType == value::ValueType::OBJECT)) {
            return true;
        }
        return false;
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
        // Skip validation if we don't know both types
        if (leftType == value::ValueType::VOID || rightType == value::ValueType::VOID) {
            return;
        }

        bool isValid = false;

        // Arithmetic operations: +, -, *, /, %
        if (op == token::TokenType::PLUS || op == token::TokenType::MINUS ||
            op == token::TokenType::MULTIPLY || op == token::TokenType::DIVIDE ||
            op == token::TokenType::MODULO) {
            isValid = isArithmeticOperationValid(leftType, rightType, op);
        }
        // Comparison operations: ==, !=, <, >, <=, >=
        else if (op == token::TokenType::EQUALS || op == token::TokenType::NOT_EQUALS ||
                 op == token::TokenType::LESS || op == token::TokenType::GREATER ||
                 op == token::TokenType::LESS_EQUALS || op == token::TokenType::GREATER_EQUALS) {
            isValid = isComparisonOperationValid(leftType, rightType, op, leftIsNull, rightIsNull);
        }
        // Logical operations: &&, ||
        else if (op == token::TokenType::AND || op == token::TokenType::OR) {
            isValid = isLogicalOperationValid(leftType, rightType);
        }
        else {
            // Unknown operator, allow it
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
        // Skip validation if we don't know both types
        if (leftType == value::ValueType::VOID || rightType == value::ValueType::VOID) {
            return;
        }

        bool isValid = false;

        // Arithmetic operations: +, -, *, /, %
        if (op == token::TokenType::PLUS || op == token::TokenType::MINUS ||
            op == token::TokenType::MULTIPLY || op == token::TokenType::DIVIDE ||
            op == token::TokenType::MODULO) {
            isValid = isArithmeticOperationValid(leftType, rightType, op);
        }
        // Comparison operations: ==, !=, <, >, <=, >=
        else if (op == token::TokenType::EQUALS || op == token::TokenType::NOT_EQUALS ||
                 op == token::TokenType::LESS || op == token::TokenType::GREATER ||
                 op == token::TokenType::LESS_EQUALS || op == token::TokenType::GREATER_EQUALS) {
            isValid = isComparisonOperationValid(leftType, leftClassName, rightType, rightClassName, op, leftIsNull, rightIsNull);
        }
        // Logical operations: &&, ||
        else if (op == token::TokenType::AND || op == token::TokenType::OR) {
            isValid = isLogicalOperationValid(leftType, rightType);
        }
        else {
            // Unknown operator, allow it
            isValid = true;
        }

        if (!isValid) {
            throwBinaryOperationError(leftType, rightType, op, location);
        }
    }
}
