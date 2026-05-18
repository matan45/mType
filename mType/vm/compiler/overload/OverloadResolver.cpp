#include "OverloadResolver.hpp"
#include <cstddef>
#include "../../../environment/registry/TypeCatalog.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include <algorithm>
#include <climits>
namespace vm::compiler::overload
{
    // ParameterConversion implementation
    bool ParameterConversion::isBetterThan(const ParameterConversion& other) const
    {
        if (!isValid() || !other.isValid()) {
            return isValid();  // Valid is better than invalid
        }

        // Compare conversion types first
        if (conversionType != other.conversionType) {
            return conversionType < other.conversionType;
        }

        // Same conversion type - compare inheritance distance
        return inheritanceDistance < other.inheritanceDistance;
    }

    // CandidateMatch implementation
    bool CandidateMatch::isViable() const
    {
        // All conversions must be valid
        // Empty conversions (0 parameters) is also viable
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
            // Weight conversion type heavily, add inheritance distance
            totalScore += static_cast<int>(conversion.conversionType) * 1000 + conversion.inheritanceDistance;
        }
    }

    bool CandidateMatch::isBetterThan(const CandidateMatch& other) const
    {
        if (!isViable() || !other.isViable()) {
            return isViable();
        }

        // Compare parameter by parameter - better if ANY parameter is better and NONE are worse
        bool hasBetter = false;
        for (size_t i = 0; i < conversions.size() && i < other.conversions.size(); ++i) {
            if (conversions[i].isBetterThan(other.conversions[i])) {
                hasBetter = true;
            } else if (other.conversions[i].isBetterThan(conversions[i])) {
                return false;  // This match has a worse conversion
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

    // Helper function to parse type arguments from a parameterized type like "Mapper<Bool, String>"
    std::vector<std::string> parseTypeArguments(const std::string& parameterizedType) {
        std::vector<std::string> args;
        size_t openAngle = parameterizedType.find('<');
        size_t closeAngle = parameterizedType.rfind('>');

        if (openAngle == std::string::npos || closeAngle == std::string::npos || closeAngle <= openAngle) {
            return args;  // Not a parameterized type
        }

        std::string typeArgsStr = parameterizedType.substr(openAngle + 1, closeAngle - openAngle - 1);

        // Parse comma-separated type arguments
        std::string currentArg;
        int depth = 0;  // Track nested angle brackets like "Box<Pair<Int, String>>"

        for (char c : typeArgsStr) {
            if (c == '<') {
                depth++;
                currentArg += c;
            } else if (c == '>') {
                depth--;
                currentArg += c;
            } else if (c == ',' && depth == 0) {
                // Found a top-level comma separator
                // Trim spaces from currentArg
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

        // Add last argument
        if (!currentArg.empty()) {
            size_t start = currentArg.find_first_not_of(' ');
            size_t end = currentArg.find_last_not_of(' ');
            if (start != std::string::npos && end != std::string::npos) {
                args.push_back(currentArg.substr(start, end - start + 1));
            }
        }

        return args;
    }

    // Helper function to check if a type name is a generic parameter (like T, K, V)
    static bool isGenericParameter(const std::string& typeName,
                                   const environment::registry::TypeCatalog& catalog) {
        // Single uppercase letter = generic parameter
        if (typeName.length() == 1 && std::isupper(typeName[0])) {
            return true;
        }
        // Two-letter type that isn't registered
        if (typeName.length() == 2 && std::isupper(typeName[0]) && !catalog.hasType(typeName)) {
            return true;
        }
        return false;
    }

    // OverloadResolver implementation
    ConversionType OverloadResolver::getConversionType(
        const value::ParameterType& from,
        const value::ParameterType& to,
        const environment::registry::TypeCatalog& catalog)
    {
        // Check for generic type parameter FIRST (before exact match)
        // Generic parameters like T, K, V should be detected before any other checks
        // This ensures they get the lowest priority (GENERIC_PARAMETER = 10)
        if (to.basicType == value::ValueType::OBJECT && to.className.has_value()) {
            const std::string& className = to.className.value();

            // Extract base class name (before '<' if it exists)
            std::string baseClassName = className;
            size_t anglePos = className.find('<');
            if (anglePos != std::string::npos) {
                baseClassName = className.substr(0, anglePos);
            }

            // Generic type parameter detection:
            // Case 1: Simple generic like "T", "K", "V" (single capital letters)
            if (isGenericParameter(baseClassName, catalog)) {
                // This is a generic parameter - can match ANY type but with lowest priority
                return ConversionType::GENERIC_PARAMETER;
            }

            // Case 2: Parameterized type containing generic parameters like "Mapper<K, String>"
            if (anglePos != std::string::npos) {

                // Check if `from` is also a parameterized type with the same base class
                if (from.basicType == value::ValueType::OBJECT && from.className.has_value()) {
                    const std::string& fromClassName = from.className.value();
                    std::string fromBaseClass = fromClassName;
                    size_t fromAngle = fromClassName.find('<');
                    if (fromAngle != std::string::npos) {
                        fromBaseClass = fromClassName.substr(0, fromAngle);
                    }

                    if (fromBaseClass == baseClassName) {
                        // Same base class - perform structural comparison of type arguments
                        auto fromArgs = parseTypeArguments(fromClassName);
                        auto toArgs = parseTypeArguments(className);

                        if (fromArgs.size() != toArgs.size()) {
                            // Mismatched type argument count - incompatible
                            return ConversionType::INCOMPATIBLE;
                        }

                        // Compare type arguments element by element
                        bool allExact = true;

                        for (size_t i = 0; i < fromArgs.size(); ++i) {
                            const std::string& fromArg = fromArgs[i];
                            const std::string& toArg = toArgs[i];

                            if (isGenericParameter(toArg, catalog)) {
                                // Target is generic parameter - matches any concrete type
                                allExact = false;
                            } else if (fromArg == toArg) {
                                // Exact match on this type argument
                            } else {
                                // Concrete type mismatch - incompatible
                                return ConversionType::INCOMPATIBLE;
                            }
                        }

                        if (allExact) {
                            return ConversionType::EXACT_MATCH;
                        } else {
                            // Return GENERIC_PARAMETER but use distance to track number of generics
                            // Note: We can't directly set distance here, but this will be handled
                            // in analyzeParameterConversion by checking the type structure
                            return ConversionType::GENERIC_PARAMETER;
                        }
                    }
                }
            }
        }

        // Also check for legacy generic representation (OBJECT without className)
        if (to.basicType == value::ValueType::OBJECT && !to.className.has_value() && !to.interfaceName.has_value()) {
            return ConversionType::GENERIC_PARAMETER;
        }

        // Exact match - highest priority
        if (from == to) {
            return ConversionType::EXACT_MATCH;
        }

        // Numeric widening: int -> float
        if (from.basicType == value::ValueType::INT && to.basicType == value::ValueType::FLOAT) {
            return ConversionType::NUMERIC_WIDENING;
        }

        // Auto-boxing: primitive -> wrapper class
        // Check if 'from' is primitive and 'to' is the corresponding box class
        if (from.basicType != value::ValueType::OBJECT && to.basicType == value::ValueType::OBJECT) {
            std::string fromTypeName = types::TypeConversionUtils::getTypeDisplayName(from.basicType);
            std::string boxClassName = catalog.getBoxClassName(fromTypeName);

            if (to.isClass() && boxClassName == to.getClassName()) {
                return ConversionType::AUTO_BOXING;
            }
        }

        // Inheritance: child class -> parent class
        // Strip nullable suffix from class names — non-null is assignable to nullable.
        // Nullability is encoded inconsistently across the codebase: some paths set
        // ParameterType.nullable=true, others append '?' to className via GenericType::toString
        // (see ClassRegistrar.cpp:218). Normalize here so the subtype lookup works either way.
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
            // Untyped object (no className) is assignable to Object (the root class)
            if (!from.isClass() && !from.isInterface() && to.isClass() &&
                types::TypeConversionUtils::stripNullable(to.getClassName()) == "Object") {
                return ConversionType::INHERITANCE;
            }
        }

        // Null can be assigned to any object or array
        if (from.basicType == value::ValueType::NULL_TYPE) {
            if (to.basicType == value::ValueType::OBJECT || to.basicType == value::ValueType::ARRAY) {
                return ConversionType::EXACT_MATCH;  // Treat null as exact match for references
            }
        }

        // Array and Object compatibility
        // For parameterized generic types (like Box<Int> vs Box<String>), require exact match
        if (from.basicType == value::ValueType::OBJECT && to.basicType == value::ValueType::OBJECT) {
            if (from.className.has_value() && to.className.has_value()) {
                // Check if both have generic type parameters (contain '<')
                bool fromHasGenericParams = from.className.value().find('<') != std::string::npos;
                bool toHasGenericParams = to.className.value().find('<') != std::string::npos;

                if (fromHasGenericParams && toHasGenericParams) {
                    // Both are parameterized types - require exact match
                    if (from.className == to.className && from.interfaceName == to.interfaceName) {
                        return ConversionType::EXACT_MATCH;
                    }
                    // Different parameterized types (e.g., Box<Int> vs Box<String>) = incompatible
                    return ConversionType::INCOMPATIBLE;
                }
            }
        }

        // For primitive types, use areTypesCompatible
        // Note: Don't use this for OBJECT types - we've already checked exact match and inheritance above
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
            // For generic parameter conversions, calculate distance based on number of generic type parameters
            // Fewer generic parameters = better match (lower distance)
            if (parameterType.basicType == value::ValueType::OBJECT && parameterType.className.has_value()) {
                const std::string& paramClassName = parameterType.className.value();

                // If this is a parameterized type, count the number of generic type parameters
                if (paramClassName.find('<') != std::string::npos) {
                    auto typeArgs = parseTypeArguments(paramClassName);

                    // Count how many are generic parameters
                    for (const auto& arg : typeArgs) {
                        if (isGenericParameter(arg, catalog)) {
                            distance++;
                        }
                    }
                } else {
                    // Simple generic parameter like "T" - distance = 1
                    distance = 1;
                }
            } else {
                // Simple generic parameter - distance = 1
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
        // Convert type names to ParameterTypes
        auto argType = typeNameToParameterType(argumentTypeName);
        auto paramType = typeNameToParameterType(parameterTypeName);

        return analyzeParameterConversion(argType, paramType, catalog);
    }

    value::ParameterType OverloadResolver::typeNameToParameterType(const std::string& typeName)
    {
        // Check for primitive types
        if (typeName == "int") return value::ParameterType(value::ValueType::INT);
        if (typeName == "float") return value::ParameterType(value::ValueType::FLOAT);
        if (typeName == "bool") return value::ParameterType(value::ValueType::BOOL);
        if (typeName == "string") return value::ParameterType(value::ValueType::STRING);
        if (typeName == "void") return value::ParameterType(value::ValueType::VOID);
        if (typeName == "null") return value::ParameterType(value::ValueType::NULL_TYPE);

        // Assume it's a class type
        return value::ParameterType::forClass(typeName);
    }

    template<typename T>
    CandidateMatch OverloadResolver::matchCandidate(
        const std::shared_ptr<T>& candidate,
        const std::vector<value::ParameterType>& argumentTypes,
        const environment::registry::TypeCatalog& catalog)
    {
        CandidateMatch match;
        const auto& parameters = getParameters(candidate);

        // Check parameter count
        if (parameters.size() != argumentTypes.size()) {
            match.totalScore = INT_MAX;
            return match;
        }

        // Analyze each parameter conversion
        match.conversions.reserve(argumentTypes.size());
        for (size_t i = 0; i < argumentTypes.size(); ++i) {
            auto conversion = analyzeParameterConversion(argumentTypes[i], parameters[i].second, catalog);
            match.conversions.push_back(conversion);
        }

        match.calculateScore();
        return match;
    }

    template<typename T>
    std::vector<std::pair<std::shared_ptr<T>, CandidateMatch>> OverloadResolver::filterViableCandidates(
        const std::vector<std::shared_ptr<T>>& candidates,
        const std::vector<value::ParameterType>& argumentTypes,
        const environment::registry::TypeCatalog& catalog)
    {
        std::vector<std::pair<std::shared_ptr<T>, CandidateMatch>> viableMatches;

        for (const auto& candidate : candidates) {
            CandidateMatch match = matchCandidate(candidate, argumentTypes, catalog);
            // A match is viable if conversions are valid AND totalScore is not INT_MAX (parameter count matches)
            if (match.isViable() && match.totalScore != INT_MAX) {
                viableMatches.emplace_back(candidate, match);
            }
        }

        return viableMatches;
    }

    template<typename T>
    OverloadResolutionResult<T> OverloadResolver::selectBestCandidate(
        const std::vector<std::pair<std::shared_ptr<T>, CandidateMatch>>& viableMatches)
    {
        OverloadResolutionResult<T> result;

        if (viableMatches.empty()) {
            result.hasViableCandidates = false;
            return result;
        }

        result.hasViableCandidates = true;

        // Find the best match(es)
        std::vector<std::pair<std::shared_ptr<T>, CandidateMatch>> bestMatches;
        bestMatches.push_back(viableMatches[0]);

        for (size_t i = 1; i < viableMatches.size(); ++i) {
            const auto& current = viableMatches[i];
            const auto& best = bestMatches[0];

            if (current.second.isBetterThan(best.second)) {
                // Found a strictly better match - replace all previous bests
                bestMatches.clear();
                bestMatches.push_back(current);
            } else if (!best.second.isBetterThan(current.second)) {
                // Neither candidate dominates the other (e.g. one wins on
                // param[0], the other on param[1] — compute(int,float) vs
                // compute(float,int) called with (int,int)). That's ambiguity,
                // not "pick the first one". The earlier isEquivalentTo gate
                // missed the mixed-conversion case because it only compared
                // matching ConversionType+distance per slot.
                bestMatches.push_back(current);
            }
        }

        // Check for ambiguity
        if (bestMatches.size() > 1) {
            result.isAmbiguous = true;
            for (const auto& match : bestMatches) {
                result.ambiguousCandidates.push_back(match.first);
            }
        } else {
            result.isAmbiguous = false;
            result.selectedOverload = bestMatches[0].first;
        }

        // Collect other viable candidates
        for (const auto& match : viableMatches) {
            bool isBest = false;
            for (const auto& best : bestMatches) {
                if (match.first == best.first) {
                    isBest = true;
                    break;
                }
            }
            if (!isBest) {
                result.viableCandidates.push_back(match.first);
            }
        }

        return result;
    }

    std::vector<std::pair<std::string, value::ParameterType>> OverloadResolver::getParameters(
        const std::shared_ptr<runtimeTypes::klass::MethodDefinition>& method)
    {
        const auto& allParams = method->getParametersWithTypes();

        // For instance methods, skip the first parameter ('this')
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

    // Public API implementations
    OverloadResolutionResult<runtimeTypes::klass::MethodDefinition> OverloadResolver::resolveMethodOverload(
        const std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>>& candidates,
        const std::vector<value::ParameterType>& argumentTypes,
        const ast::SourceLocation& sourceLocation,
        const environment::registry::TypeCatalog& catalog)
    {
        auto viableMatches = filterViableCandidates(candidates, argumentTypes, catalog);
        return selectBestCandidate(viableMatches);
    }

    OverloadResolutionResult<runtimeTypes::klass::MethodDefinition> OverloadResolver::resolveMethodOverload(
        const std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>>& candidates,
        const std::vector<std::string>& argumentTypeNames,
        const ast::SourceLocation& sourceLocation,
        const environment::registry::TypeCatalog& catalog)
    {
        std::vector<value::ParameterType> argumentTypes;
        argumentTypes.reserve(argumentTypeNames.size());
        for (const auto& typeName : argumentTypeNames) {
            argumentTypes.push_back(typeNameToParameterType(typeName));
        }

        return resolveMethodOverload(candidates, argumentTypes, sourceLocation, catalog);
    }

    OverloadResolutionResult<runtimeTypes::global::FunctionDefinition> OverloadResolver::resolveFunctionOverload(
        const std::vector<std::shared_ptr<runtimeTypes::global::FunctionDefinition>>& candidates,
        const std::vector<value::ParameterType>& argumentTypes,
        const ast::SourceLocation& sourceLocation,
        const environment::registry::TypeCatalog& catalog)
    {
        auto viableMatches = filterViableCandidates(candidates, argumentTypes, catalog);
        return selectBestCandidate(viableMatches);
    }

    OverloadResolutionResult<runtimeTypes::global::FunctionDefinition> OverloadResolver::resolveFunctionOverload(
        const std::vector<std::shared_ptr<runtimeTypes::global::FunctionDefinition>>& candidates,
        const std::vector<std::string>& argumentTypeNames,
        const ast::SourceLocation& sourceLocation,
        const environment::registry::TypeCatalog& catalog)
    {
        std::vector<value::ParameterType> argumentTypes;
        argumentTypes.reserve(argumentTypeNames.size());
        for (const auto& typeName : argumentTypeNames) {
            argumentTypes.push_back(typeNameToParameterType(typeName));
        }

        return resolveFunctionOverload(candidates, argumentTypes, sourceLocation, catalog);
    }

} // namespace vm::compiler::overload
