#include "TypeValidator.hpp"
#include <algorithm>
#include <cstddef>
#include "../../../errors/TypeException.hpp"
#include "../../../types/TypeConversionUtils.hpp"

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
        if (visited.count(interfaceName)) {
            return false;
        }
        visited.insert(interfaceName);

        std::string interfaceBaseName = stripGenericParameters(interfaceName);
        std::string targetBaseName = stripGenericParameters(targetInterface);

        if (interfaceBaseName == targetBaseName) {
            return true;
        }

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

        // Strip generic type parameters: "InMemoryRepository<User>" -> "InMemoryRepository"
        std::string derivedBaseName = derivedClass;
        size_t derivedGenericStart = derivedClass.find('<');
        if (derivedGenericStart != std::string::npos) {
            derivedBaseName = derivedClass.substr(0, derivedGenericStart);
        }

        std::string baseBaseName = baseClass;
        size_t baseGenericStart = baseClass.find('<');
        if (baseGenericStart != std::string::npos) {
            baseBaseName = baseClass.substr(0, baseGenericStart);
        }

        if (derivedBaseName == baseBaseName) {
            return true;
        }

        auto classDef = environment->findClass(derivedBaseName);
        if (classDef) {
            // Walk parent chain, checking implemented interfaces at each level.
            auto currentClass = classDef;
            while (currentClass) {
                const auto& interfaces = currentClass->getImplementedInterfaces();
                for (const auto& interfaceName : interfaces) {
                    std::string interfaceBaseName = interfaceName;
                    size_t interfaceGenericStart = interfaceName.find('<');
                    if (interfaceGenericStart != std::string::npos) {
                        interfaceBaseName = interfaceName.substr(0, interfaceGenericStart);
                    }

                    std::unordered_set<std::string> visited;
                    if (checkInterfaceHierarchy(interfaceBaseName, baseBaseName, visited)) {
                        return true;
                    }
                }

                auto parentClass = currentClass->getParentClass();
                if (parentClass) {
                    std::string parentName = parentClass->getName();
                    std::string parentBaseName = parentName;
                    size_t parentGenericStart = parentName.find('<');
                    if (parentGenericStart != std::string::npos) {
                        parentBaseName = parentName.substr(0, parentGenericStart);
                    }

                    if (parentBaseName == baseBaseName) {
                        return true;
                    }
                }
                currentClass = parentClass;
            }

            return false;
        }

        // derivedClass may be an interface that extends baseClass
        auto interfaceDef = environment->findInterface(derivedBaseName);
        if (interfaceDef) {
            std::unordered_set<std::string> visited;
            if (checkInterfaceHierarchy(derivedBaseName, baseBaseName, visited)) {
                return true;
            }
        }

        return false;
    }

    std::string TypeValidator::normalizeArrayType(const std::string& type) const
    {
        std::string normalized = type;
        size_t arrayDepth = 0;

        while (normalized.length() >= 2 && normalized.substr(normalized.length() - 2) == "[]") {
            arrayDepth++;
            normalized = normalized.substr(0, normalized.length() - 2);
        }

        for (size_t i = 0; i < arrayDepth; ++i) {
            normalized = "Array<" + normalized + ">";
        }

        return normalized;
    }

    bool TypeValidator::validatePromiseTypeAssignment(const std::string& varClassName, const std::string& valueClassName) const
    {
        // Promise<T> accepts T (async functions auto-wrap).
        if (varClassName.find("Promise<") != 0) {
            return false;
        }

        size_t start = varClassName.find('<') + 1;
        size_t end = varClassName.rfind('>');
        if (start == std::string::npos || end == std::string::npos || end <= start) {
            return false;
        }

        std::string innerType = varClassName.substr(start, end - start);
        innerType.erase(0, innerType.find_first_not_of(" \t"));
        innerType.erase(innerType.find_last_not_of(" \t") + 1);

        if (valueClassName == innerType) {
            return true;
        }

        if (stripGenericParameters(valueClassName) == stripGenericParameters(innerType)) {
            return true;
        }

        std::string normalizedInnerType = normalizeArrayType(innerType);
        std::string normalizedValueClass = normalizeArrayType(valueClassName);

        return (normalizedValueClass == normalizedInnerType ||
                stripGenericParameters(normalizedValueClass) == stripGenericParameters(normalizedInnerType));
    }

    void TypeValidator::validateObjectTypeAssignment(const std::string& varClassName, const std::string& valueClassName,
                                                     const ast::SourceLocation& location) const
    {
        // Non-nullable T is always assignable to nullable T?, so strip suffix first.
        std::string varClass = ::types::TypeConversionUtils::stripNullable(varClassName);
        std::string valueClass = ::types::TypeConversionUtils::stripNullable(valueClassName);

        if (varClass == valueClass)
            return;

        if ((varClass == "Array" || varClass.find("Array<") == 0) &&
            valueClass.find("[]") != std::string::npos) {
            return;
        }

        if (validatePromiseTypeAssignment(varClass, valueClass)) {
            return;
        }

        // MYT-137: strict declaration-time array invariance. Element types
        // must match EXACTLY at the assignment site — `Animal[] a = new Dog[1]`
        // and `Animal[] a = dogs` (where dogs is Dog[]) are both rejected at
        // compile time. The historical covariant escape (allow when value's
        // element is a subtype of var's) is gone; that masks the array-store
        // soundness hole because once the alias is established the runtime
        // can no longer tell that the storage is actually Dog-typed.
        //
        // Array literals like `Shape[] shapes = [new A(), new B(), new C()]`
        // remain valid: StatementCompiler's decl path validates each element
        // against the declared element type and retypes the literal to the
        // declared array type before reaching this check (see
        // StatementCompiler.cpp ~line 892, MYT-137 target-type-guided inference).
        bool varIsArray = (varClass.find("[]") != std::string::npos);
        bool valueIsArray = (valueClass.find("[]") != std::string::npos);

        if (varIsArray && valueIsArray) {
            std::string varElementType = varClass.substr(0, varClass.find("[]"));
            std::string valueElementType = valueClass.substr(0, valueClass.find("[]"));

            // Generic-arg whitespace is not load-bearing: the parser preserves
            // user-written spacing ("Pair<String, Int>") while UnifiedType /
            // bytecode emit a no-space canonical ("Pair<String,Int>"). Both
            // refer to the same reified type. Strip all spaces before the
            // strict comparison so identical types don't trip the invariance
            // check on cosmetic differences. mType identifiers contain no
            // spaces, so stripping is safe.
            auto stripSpaces = [](std::string s) {
                s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
                return s;
            };
            if (stripSpaces(varElementType) != stripSpaces(valueElementType)) {
                throw errors::TypeException(
                    "Array covariance violation: cannot assign " + valueElementType + "[] to " +
                    varElementType + "[] (array element types are invariant; covariance must be "
                    "explicit via array literal or runtime cast)",
                    location
                );
            }
            return;
        }

        std::string normalizedVarClass = normalizeArrayType(varClass);
        std::string normalizedValueClass = normalizeArrayType(valueClass);

        std::string baseVarClass = stripGenericParameters(normalizedVarClass);
        std::string baseValueClass = stripGenericParameters(normalizedValueClass);

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

        // PHASE 4: primitive-to-Box auto-boxing.
        if ((varClassName == "Int" && valueType == value::ValueType::INT) ||
            (varClassName == "Float" && valueType == value::ValueType::FLOAT) ||
            (varClassName == "Bool" && valueType == value::ValueType::BOOL) ||
            (varClassName == "String" && valueType == value::ValueType::STRING)) {
            return;
        }

        // MYT-281: arrays widen to `Object` (the universal heap-typed slot).
        // The runtime preserves the array bridge through Object-typed
        // variables, fields, and parameters; round-trip narrowing goes
        // through `(T[])obj` which validates element type at the cast.
        if (varClassName == "Object" && valueType == value::ValueType::ARRAY) {
            return;
        }

        // Unresolved generic object slots (T, V, Box<T>, etc.) are validated
        // when concrete bindings are available. Do not reject primitive values
        // here based only on the placeholder name.
        if (::types::TypeConversionUtils::containsGenericTypeParameter(varClassName)) {
            return;
        }

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

        // INT widens to FLOAT.
        if (valueType == value::ValueType::INT && varType == value::ValueType::FLOAT) {
            return;
        }

        // String class object → string primitive.
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
        const ast::SourceLocation& location,
        bool isNullableTarget
    ) const
    {
        if (isNullValue && varType == value::ValueType::OBJECT) {
            if (isNullableTarget) {
                return;
            }
            throw errors::TypeException(
                "Cannot assign null to non-nullable type '" + varClassName + "'. "
                "Use '" + varClassName + "?' to make it nullable.",
                location);
        }

        // Arrays may be tagged as ARRAY or OBJECT depending on context.
        bool varIsArray = (varType == value::ValueType::ARRAY) ||
                         (varType == value::ValueType::OBJECT && !varClassName.empty() &&
                          (varClassName.find("[]") != std::string::npos || varClassName.find("Array<") == 0));
        bool valueIsArray = (valueType == value::ValueType::ARRAY) ||
                           (valueType == value::ValueType::OBJECT && !valueClassName.empty() &&
                            (valueClassName.find("[]") != std::string::npos || valueClassName.find("Array<") == 0));

        if (varIsArray && valueIsArray) {
            if (!varClassName.empty() && !valueClassName.empty()) {
                validateObjectTypeAssignment(varClassName, valueClassName, location);
            }
            return;
        }

        if (varType == value::ValueType::OBJECT && valueType == value::ValueType::OBJECT) {
            if (!varClassName.empty() && !valueClassName.empty()) {
                validateObjectTypeAssignment(varClassName, valueClassName, location);
            }
            return;
        }

        validatePrimitiveToObjectAssignment(varType, varClassName, valueType, location);

        // PHASE 4: Box object → primitive auto-unboxing.
        if (varType != value::ValueType::OBJECT && valueType == value::ValueType::OBJECT)
        {
            if ((varType == value::ValueType::INT && valueClassName == "Int") ||
                (varType == value::ValueType::FLOAT && valueClassName == "Float") ||
                (varType == value::ValueType::BOOL && valueClassName == "Bool") ||
                (varType == value::ValueType::STRING && valueClassName == "String"))
            {
                return;
            }
        }

        validatePrimitiveTypeAssignment(varType, valueType, location);
    }
}
