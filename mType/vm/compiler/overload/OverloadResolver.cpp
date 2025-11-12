#include "OverloadResolver.hpp"
#include "../../../types/TypeRegistry.hpp"
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

    // OverloadResolver implementation
    ConversionType OverloadResolver::getConversionType(
        const value::ParameterType& from,
        const value::ParameterType& to)
    {
        // Check for generic type parameter FIRST (before exact match)
        // Generic parameters like T, K, V should be detected before any other checks
        // This ensures they get the lowest priority (GENERIC_PARAMETER = 10)
        if (to.basicType == value::ValueType::OBJECT && to.className.has_value()) {
            auto& registry = types::getGlobalTypeRegistry();
            const std::string& className = to.className.value();

            // Extract base class name (before '<' if it exists)
            std::string baseClassName = className;
            size_t anglePos = className.find('<');
            if (anglePos != std::string::npos) {
                baseClassName = className.substr(0, anglePos);
            }

            // Generic type parameter detection:
            // Case 1: Simple generic like "T", "K", "V" (single capital letters)
            if (baseClassName.length() <= 2 && std::isupper(baseClassName[0]) && !registry.hasType(baseClassName)) {
                // This is a generic parameter - can match ANY type but with lowest priority
                return ConversionType::GENERIC_PARAMETER;
            }

            // Case 2: Parameterized type containing generic parameters like "Box<T>", "List<K>"
            // Check if className contains angle brackets with unresolved type parameters
            size_t openAngle = className.find('<');
            if (openAngle != std::string::npos) {
                size_t closeAngle = className.find('>');
                if (closeAngle != std::string::npos && closeAngle > openAngle) {
                    // Extract the type argument(s) between < and >
                    std::string typeArgs = className.substr(openAngle + 1, closeAngle - openAngle - 1);

                    // Simple heuristic: if type args contain single letters (T, K, V, etc.)
                    // or comma-separated single letters, it's likely generic
                    bool hasGenericParams = false;
                    std::string trimmedArg;
                    for (char c : typeArgs) {
                        if (c != ' ' && c != ',') {
                            trimmedArg += c;
                        } else if (!trimmedArg.empty()) {
                            // Check if this is a simple generic parameter
                            if (trimmedArg.length() <= 2 && !registry.hasType(trimmedArg)) {
                                hasGenericParams = true;
                                break;
                            }
                            trimmedArg.clear();
                        }
                    }
                    // Check last arg
                    if (!trimmedArg.empty() && trimmedArg.length() <= 2 && !registry.hasType(trimmedArg)) {
                        hasGenericParams = true;
                    }

                    if (hasGenericParams) {
                        return ConversionType::GENERIC_PARAMETER;
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
        auto& registry = types::getGlobalTypeRegistry();

        // Check if 'from' is primitive and 'to' is the corresponding box class
        if (from.basicType != value::ValueType::OBJECT && to.basicType == value::ValueType::OBJECT) {
            std::string fromTypeName = types::TypeConversionUtils::getTypeDisplayName(from.basicType);
            std::string boxClassName = registry.getBoxClassName(fromTypeName);

            if (to.isClass() && boxClassName == to.getClassName()) {
                return ConversionType::AUTO_BOXING;
            }
        }

        // Inheritance: child class -> parent class
        if (from.basicType == value::ValueType::OBJECT && to.basicType == value::ValueType::OBJECT) {
            if (from.isClass() && to.isClass()) {
                if (registry.isSubtypeOf(from.getClassName(), to.getClassName())) {
                    return ConversionType::INHERITANCE;
                }
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

        // For non-parameterized types, use areTypesCompatible
        if (types::TypeConversionUtils::areTypesCompatible(from.basicType, to.basicType)) {
            return ConversionType::EXACT_MATCH;
        }

        return ConversionType::INCOMPATIBLE;
    }

    int OverloadResolver::calculateInheritanceDistance(
        const std::string& childType,
        const std::string& parentType)
    {
        if (childType == parentType) {
            return 0;
        }

        auto& registry = types::getGlobalTypeRegistry();
        if (!registry.isSubtypeOf(childType, parentType)) {
            return -1;
        }

        // Walk up the inheritance chain counting steps
        std::string current = childType;
        int distance = 0;
        const int MAX_DEPTH = 20;  // Prevent infinite loops

        // Note: TypeRegistry doesn't expose the inheritance map directly
        // For now, we return a fixed distance if subtype relationship exists
        // A more precise implementation would walk the chain
        return 1;  // Simplified: just return 1 for any inheritance relationship
    }

    ParameterConversion OverloadResolver::analyzeParameterConversion(
        const value::ParameterType& argumentType,
        const value::ParameterType& parameterType)
    {
        ConversionType convType = getConversionType(argumentType, parameterType);

        if (convType == ConversionType::INCOMPATIBLE) {
            return ParameterConversion(ConversionType::INCOMPATIBLE, 0);
        }

        int distance = 0;
        if (convType == ConversionType::INHERITANCE) {
            if (argumentType.isClass() && parameterType.isClass()) {
                distance = calculateInheritanceDistance(
                    argumentType.getClassName(),
                    parameterType.getClassName());
            }
        }

        return ParameterConversion(convType, distance);
    }

    ParameterConversion OverloadResolver::analyzeParameterConversion(
        const std::string& argumentTypeName,
        const std::string& parameterTypeName)
    {
        // Convert type names to ParameterTypes
        auto argType = typeNameToParameterType(argumentTypeName);
        auto paramType = typeNameToParameterType(parameterTypeName);

        return analyzeParameterConversion(argType, paramType);
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
        const std::vector<value::ParameterType>& argumentTypes)
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
            auto conversion = analyzeParameterConversion(argumentTypes[i], parameters[i].second);
            match.conversions.push_back(conversion);
        }

        match.calculateScore();
        return match;
    }

    template<typename T>
    std::vector<std::pair<std::shared_ptr<T>, CandidateMatch>> OverloadResolver::filterViableCandidates(
        const std::vector<std::shared_ptr<T>>& candidates,
        const std::vector<value::ParameterType>& argumentTypes)
    {
        std::vector<std::pair<std::shared_ptr<T>, CandidateMatch>> viableMatches;

        for (const auto& candidate : candidates) {
            CandidateMatch match = matchCandidate(candidate, argumentTypes);
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
                // Equally good - potential ambiguity
                if (current.second.isEquivalentTo(best.second)) {
                    bestMatches.push_back(current);
                }
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
        const ast::SourceLocation& sourceLocation)
    {
        auto viableMatches = filterViableCandidates(candidates, argumentTypes);
        return selectBestCandidate(viableMatches);
    }

    OverloadResolutionResult<runtimeTypes::klass::MethodDefinition> OverloadResolver::resolveMethodOverload(
        const std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>>& candidates,
        const std::vector<std::string>& argumentTypeNames,
        const ast::SourceLocation& sourceLocation)
    {
        std::vector<value::ParameterType> argumentTypes;
        argumentTypes.reserve(argumentTypeNames.size());
        for (const auto& typeName : argumentTypeNames) {
            argumentTypes.push_back(typeNameToParameterType(typeName));
        }

        return resolveMethodOverload(candidates, argumentTypes, sourceLocation);
    }

    OverloadResolutionResult<runtimeTypes::global::FunctionDefinition> OverloadResolver::resolveFunctionOverload(
        const std::vector<std::shared_ptr<runtimeTypes::global::FunctionDefinition>>& candidates,
        const std::vector<value::ParameterType>& argumentTypes,
        const ast::SourceLocation& sourceLocation)
    {
        auto viableMatches = filterViableCandidates(candidates, argumentTypes);
        return selectBestCandidate(viableMatches);
    }

    OverloadResolutionResult<runtimeTypes::global::FunctionDefinition> OverloadResolver::resolveFunctionOverload(
        const std::vector<std::shared_ptr<runtimeTypes::global::FunctionDefinition>>& candidates,
        const std::vector<std::string>& argumentTypeNames,
        const ast::SourceLocation& sourceLocation)
    {
        std::vector<value::ParameterType> argumentTypes;
        argumentTypes.reserve(argumentTypeNames.size());
        for (const auto& typeName : argumentTypeNames) {
            argumentTypes.push_back(typeNameToParameterType(typeName));
        }

        return resolveFunctionOverload(candidates, argumentTypes, sourceLocation);
    }

} // namespace vm::compiler::overload
