#include "OverloadResolver.hpp"
#include <cstddef>
#include "../../../environment/registry/TypeCatalog.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include <algorithm>
#include <climits>

namespace vm::compiler::overload
{
    bool ParameterConversion::isBetterThan(const ParameterConversion& other) const
    {
        if (!isValid() || !other.isValid()) {
            return isValid();
        }

        if (conversionType != other.conversionType) {
            return conversionType < other.conversionType;
        }

        return inheritanceDistance < other.inheritanceDistance;
    }

    bool CandidateMatch::isViable() const
    {
        // Empty conversions (0 parameters) is viable.
        for (const auto& conversion : conversions) {
            if (!conversion.isValid()) {
                return false;
            }
        }
        return true;
    }

    void CandidateMatch::calculateScore()
    {
        if (!isViable()) {
            totalScore = INT_MAX;
            return;
        }

        totalScore = 0;
        for (const auto& conversion : conversions) {
            totalScore += static_cast<int>(conversion.conversionType) * 1000 + conversion.inheritanceDistance;
        }
    }

    bool CandidateMatch::isBetterThan(const CandidateMatch& other) const
    {
        if (!isViable() || !other.isViable()) {
            return isViable();
        }

        // Better if ANY parameter is better and NONE are worse.
        bool hasBetter = false;
        for (size_t i = 0; i < conversions.size() && i < other.conversions.size(); ++i) {
            if (conversions[i].isBetterThan(other.conversions[i])) {
                hasBetter = true;
            } else if (other.conversions[i].isBetterThan(conversions[i])) {
                return false;
            }
        }
        return hasBetter;
    }

    bool CandidateMatch::isEquivalentTo(const CandidateMatch& other) const
    {
        if (conversions.size() != other.conversions.size()) {
            return false;
        }

        for (size_t i = 0; i < conversions.size(); ++i) {
            if (conversions[i].conversionType != other.conversions[i].conversionType ||
                conversions[i].inheritanceDistance != other.conversions[i].inheritanceDistance) {
                return false;
            }
        }
        return true;
    }

    // Parses type arguments from a parameterized type like "Mapper<Bool, String>".
    std::vector<std::string> parseTypeArguments(const std::string& parameterizedType) {
        std::vector<std::string> args;
        size_t openAngle = parameterizedType.find('<');
        size_t closeAngle = parameterizedType.rfind('>');

        if (openAngle == std::string::npos || closeAngle == std::string::npos || closeAngle <= openAngle) {
            return args;
        }

        std::string typeArgsStr = parameterizedType.substr(openAngle + 1, closeAngle - openAngle - 1);

        std::string currentArg;
        int depth = 0;  // Track nested angle brackets like "Box<Pair<Int, String>>".

        for (char c : typeArgsStr) {
            if (c == '<') {
                depth++;
                currentArg += c;
            } else if (c == '>') {
                depth--;
                currentArg += c;
            } else if (c == ',' && depth == 0) {
                size_t start = currentArg.find_first_not_of(' ');
                size_t end = currentArg.find_last_not_of(' ');
                if (start != std::string::npos && end != std::string::npos) {
                    args.push_back(currentArg.substr(start, end - start + 1));
                }
                currentArg.clear();
            } else {
                currentArg += c;
            }
        }

        if (!currentArg.empty()) {
            size_t start = currentArg.find_first_not_of(' ');
            size_t end = currentArg.find_last_not_of(' ');
            if (start != std::string::npos && end != std::string::npos) {
                args.push_back(currentArg.substr(start, end - start + 1));
            }
        }

        return args;
    }

    // Detects generic parameters (T, K, V): single uppercase letter, or
    // two-letter uppercase-prefixed name not registered as a real type.
    static bool isGenericParameter(const std::string& typeName,
                                   const environment::registry::TypeCatalog& catalog) {
        if (typeName.length() == 1 && std::isupper(typeName[0])) {
            return true;
        }
        if (typeName.length() == 2 && std::isupper(typeName[0]) && !catalog.hasType(typeName)) {
            return true;
        }
        return false;
    }

    ConversionType OverloadResolver::getConversionType(
        const value::ParameterType& from,
        const value::ParameterType& to,
        const environment::registry::TypeCatalog& catalog)
    {
        // Generic-parameter check must precede exact match. Single capitals
        // like T/K/V should be detected before any other check so they get
        // GENERIC_PARAMETER's lowest priority (= 10).
        if (to.basicType == value::ValueType::OBJECT && to.className.has_value()) {
            const std::string& className = to.className.value();

            std::string baseClassName = className;
            size_t anglePos = className.find('<');
            if (anglePos != std::string::npos) {
                baseClassName = className.substr(0, anglePos);
            }

            if (isGenericParameter(baseClassName, catalog)) {
                return ConversionType::GENERIC_PARAMETER;
            }

            // Parameterized type containing generic parameters like "Mapper<K, String>".
            if (anglePos != std::string::npos) {

                if (from.basicType == value::ValueType::OBJECT && from.className.has_value()) {
                    const std::string& fromClassName = from.className.value();
                    std::string fromBaseClass = fromClassName;
                    size_t fromAngle = fromClassName.find('<');
                    if (fromAngle != std::string::npos) {
                        fromBaseClass = fromClassName.substr(0, fromAngle);
                    }

                    if (fromBaseClass == baseClassName) {
                        // Same base — structural comparison of type arguments.
                        auto fromArgs = parseTypeArguments(fromClassName);
                        auto toArgs = parseTypeArguments(className);

                        if (fromArgs.size() != toArgs.size()) {
                            return ConversionType::INCOMPATIBLE;
                        }

                        bool allExact = true;

                        for (size_t i = 0; i < fromArgs.size(); ++i) {
                            const std::string& fromArg = fromArgs[i];
                            const std::string& toArg = toArgs[i];

                            if (isGenericParameter(toArg, catalog)) {
                                allExact = false;
                            } else if (fromArg == toArg) {
                                // Exact on this argument.
                            } else {
                                return ConversionType::INCOMPATIBLE;
                            }
                        }

                        if (allExact) {
                            return ConversionType::EXACT_MATCH;
                        } else {
                            // Distance is set in analyzeParameterConversion
                            // based on how many type args are generic.
                            return ConversionType::GENERIC_PARAMETER;
                        }
                    }
                }
            }
        }

        // Legacy generic representation (OBJECT without className).
        if (to.basicType == value::ValueType::OBJECT && !to.className.has_value() && !to.interfaceName.has_value()) {
            return ConversionType::GENERIC_PARAMETER;
        }

        if (from == to) {
            return ConversionType::EXACT_MATCH;
        }

        if (from.basicType == value::ValueType::INT && to.basicType == value::ValueType::FLOAT) {
            return ConversionType::NUMERIC_WIDENING;
        }

        // Auto-boxing: primitive -> wrapper class.
        if (from.basicType != value::ValueType::OBJECT && to.basicType == value::ValueType::OBJECT) {
            std::string fromTypeName = types::TypeConversionUtils::getTypeDisplayName(from.basicType);
            std::string boxClassName = catalog.getBoxClassName(fromTypeName);

            if (to.isClass() && boxClassName == to.getClassName()) {
                return ConversionType::AUTO_BOXING;
            }
        }

        // Inheritance: child class -> parent class.
        // Strip nullable suffix from class names — non-null is assignable to
        // nullable. Nullability is encoded inconsistently across the codebase:
        // some paths set ParameterType.nullable=true, others append '?' to
        // className via GenericType::toString (see ClassRegistrar.cpp:218).
        // Normalize here so the subtype lookup works either way.
        if (from.basicType == value::ValueType::OBJECT && to.basicType == value::ValueType::OBJECT) {
            if (from.isClass() && to.isClass()) {
                std::string fromBase = types::TypeConversionUtils::stripNullable(from.getClassName());
                std::string toBase = types::TypeConversionUtils::stripNullable(to.getClassName());
                if (catalog.isSubtypeOf(fromBase, toBase)) {
                    return ConversionType::INHERITANCE;
                }
                // class -> interface fallback. FunctionNode::classifyParameterType
                // stores every OBJECT-typed parameter as ParameterType::forClass
                // regardless of whether the identifier names a class or an
                // interface — so an interface-typed parameter looks like a class
                // here. After the class-subtype check misses, ask the source
                // class itself whether it implements the target name as an
                // interface. Diamond-implements (one class, two unrelated
                // interfaces, two overloads) then becomes ambiguous via the
                // equivalent-distance path in selectBestCandidate.
                if (auto classDef = catalog.findClass(fromBase)) {
                    if (classDef->implementsInterface(toBase)) {
                        return ConversionType::INHERITANCE;
                    }
                }
            }
            // If a future code path correctly stores the param as
            // ParameterType::forInterface, honour it directly.
            if (from.isClass() && to.isInterface()) {
                std::string fromBase = types::TypeConversionUtils::stripNullable(from.getClassName());
                std::string toIface = types::TypeConversionUtils::stripNullable(to.getInterfaceName());
                if (auto classDef = catalog.findClass(fromBase)) {
                    if (classDef->implementsInterface(toIface)) {
                        return ConversionType::INHERITANCE;
                    }
                }
            }
            // Untyped object (no className) is assignable to Object (root class).
            if (!from.isClass() && !from.isInterface() && to.isClass() &&
                types::TypeConversionUtils::stripNullable(to.getClassName()) == "Object") {
                return ConversionType::INHERITANCE;
            }
        }

        // null is assignable to any object or array reference (treated as exact).
        if (from.basicType == value::ValueType::NULL_TYPE) {
            if (to.basicType == value::ValueType::OBJECT || to.basicType == value::ValueType::ARRAY) {
                return ConversionType::EXACT_MATCH;
            }
        }

        // Parameterized generics (Box<Int> vs Box<String>) require exact match.
        if (from.basicType == value::ValueType::OBJECT && to.basicType == value::ValueType::OBJECT) {
            if (from.className.has_value() && to.className.has_value()) {
                bool fromHasGenericParams = from.className.value().find('<') != std::string::npos;
                bool toHasGenericParams = to.className.value().find('<') != std::string::npos;

                if (fromHasGenericParams && toHasGenericParams) {
                    if (from.className == to.className && from.interfaceName == to.interfaceName) {
                        return ConversionType::EXACT_MATCH;
                    }
                    return ConversionType::INCOMPATIBLE;
                }
            }
        }

        // Primitive-to-primitive compatibility. OBJECT was handled above.
        if (from.basicType != value::ValueType::OBJECT && to.basicType != value::ValueType::OBJECT) {
            if (types::TypeConversionUtils::areTypesCompatible(from.basicType, to.basicType)) {
                return ConversionType::EXACT_MATCH;
            }
        }

        return ConversionType::INCOMPATIBLE;
    }

    int OverloadResolver::calculateInheritanceDistance(
        const std::string& childType,
        const std::string& parentType,
        const environment::registry::TypeCatalog& catalog)
    {
        return catalog.getInheritanceDistance(childType, parentType);
    }

    ParameterConversion OverloadResolver::analyzeParameterConversion(
        const value::ParameterType& argumentType,
        const value::ParameterType& parameterType,
        const environment::registry::TypeCatalog& catalog)
    {
        ConversionType convType = getConversionType(argumentType, parameterType, catalog);

        if (convType == ConversionType::INCOMPATIBLE) {
            return ParameterConversion(ConversionType::INCOMPATIBLE, 0);
        }

        int distance = 0;
        if (convType == ConversionType::INHERITANCE) {
            if (argumentType.isClass() && parameterType.isClass()) {
                distance = calculateInheritanceDistance(
                    argumentType.getClassName(),
                    parameterType.getClassName(),
                    catalog);
            }
        } else if (convType == ConversionType::GENERIC_PARAMETER) {
            // Fewer generic parameters = better match (lower distance).
            if (parameterType.basicType == value::ValueType::OBJECT && parameterType.className.has_value()) {
                const std::string& paramClassName = parameterType.className.value();

                if (paramClassName.find('<') != std::string::npos) {
                    auto typeArgs = parseTypeArguments(paramClassName);

                    for (const auto& arg : typeArgs) {
                        if (isGenericParameter(arg, catalog)) {
                            distance++;
                        }
                    }
                } else {
                    // Simple generic parameter like "T".
                    distance = 1;
                }
            } else {
                distance = 1;
            }
        }

        return ParameterConversion(convType, distance);
    }

    ParameterConversion OverloadResolver::analyzeParameterConversion(
        const std::string& argumentTypeName,
        const std::string& parameterTypeName,
        const environment::registry::TypeCatalog& catalog)
    {
        auto argType = typeNameToParameterType(argumentTypeName);
        auto paramType = typeNameToParameterType(parameterTypeName);

        return analyzeParameterConversion(argType, paramType, catalog);
    }

    value::ParameterType OverloadResolver::typeNameToParameterType(const std::string& typeName)
    {
        if (typeName == "int") return value::ParameterType(value::ValueType::INT);
        if (typeName == "float") return value::ParameterType(value::ValueType::FLOAT);
        if (typeName == "bool") return value::ParameterType(value::ValueType::BOOL);
        if (typeName == "string") return value::ParameterType(value::ValueType::STRING);
        if (typeName == "void") return value::ParameterType(value::ValueType::VOID);
        if (typeName == "null") return value::ParameterType(value::ValueType::NULL_TYPE);

        return value::ParameterType::forClass(typeName);
    }

    std::vector<std::pair<std::string, value::ParameterType>> OverloadResolver::getParameters(
        const std::shared_ptr<runtimeTypes::klass::MethodDefinition>& method)
    {
        const auto& allParams = method->getParametersWithTypes();

        // Skip 'this' for instance methods.
        if (!method->isStatic() && !allParams.empty())
        {
            return std::vector<std::pair<std::string, value::ParameterType>>(
                allParams.begin() + 1, allParams.end());
        }

        return allParams;
    }

    const std::vector<std::pair<std::string, value::ParameterType>>& OverloadResolver::getParameters(
        const std::shared_ptr<runtimeTypes::global::FunctionDefinition>& function)
    {
        return function->getParametersWithTypes();
    }

} // namespace vm::compiler::overload
