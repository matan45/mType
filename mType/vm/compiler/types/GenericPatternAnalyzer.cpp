#include "GenericPatternAnalyzer.hpp"
#include <cstddef>
#include "../../../errors/TypeException.hpp"
#include <algorithm>

namespace vm::compiler::types
{
    bool GenericPatternAnalyzer::isTypeParameter(
        const std::string& typeName,
        const std::unordered_set<std::string>& declaredTypeParams
    )
    {
        // Type parameter must be in the declared set
        return declaredTypeParams.find(typeName) != declaredTypeParams.end();
    }

    static std::unordered_set<std::string> extractUsedTypeParametersImpl(
        const std::string& typeExpression,
        const std::unordered_set<std::string>& declaredTypeParams,
        int depth)
    {
        std::unordered_set<std::string> usedParams;

        // Guard against pathological nesting depth
        if (depth >= GenericPatternAnalyzer::MAX_EXTRACT_DEPTH)
        {
            return usedParams;
        }

        // PHASE 2 FIX: Only return type parameters that are NESTED inside generic types
        // Direct type parameters (e.g., "T" in function<T> foo(T x)) should return empty set
        // Nested type parameters (e.g., "T" in function<T> foo(Box<T> x)) should return {"T"}

        // Check if the whole expression is a direct type parameter
        if (GenericPatternAnalyzer::isTypeParameter(typeExpression, declaredTypeParams))
        {
            // Direct type parameter - return empty set (not nested)
            return usedParams;
        }

        // Extract type arguments if this is a generic type
        ::types::TypeSubstitutionService service;
        std::vector<std::string> typeArgs = service.extractTypeArguments(typeExpression);

        // Recursively check each type argument for nested type parameters
        for (const auto& arg : typeArgs)
        {
            // Check if this arg is a type parameter or contains one
            if (GenericPatternAnalyzer::isTypeParameter(arg, declaredTypeParams))
            {
                // Found a nested type parameter!
                usedParams.insert(arg);
            }
            else
            {
                // Recursively search deeper
                auto nestedParams = extractUsedTypeParametersImpl(arg, declaredTypeParams, depth + 1);
                usedParams.insert(nestedParams.begin(), nestedParams.end());
            }
        }

        return usedParams;
    }

    std::unordered_set<std::string> GenericPatternAnalyzer::extractUsedTypeParameters(
        const std::string& typeExpression,
        const std::unordered_set<std::string>& declaredTypeParams
    )
    {
        return extractUsedTypeParametersImpl(typeExpression, declaredTypeParams, 0);
    }

    std::vector<TypeBinding> GenericPatternAnalyzer::matchPattern(
        const std::string& patternType,
        const std::string& concreteType,
        const std::unordered_set<std::string>& declaredTypeParams
    ) const
    {
        std::vector<TypeBinding> bindings;

        // Perform recursive matching
        bool matches = matchPatternRecursive(patternType, concreteType, declaredTypeParams, bindings);

        if (!matches)
        {
            // Pattern doesn't match - return empty bindings
            return {};
        }

        return bindings;
    }

    bool GenericPatternAnalyzer::matchPatternRecursive(
        const std::string& pattern,
        const std::string& concrete,
        const std::unordered_set<std::string>& declaredTypeParams,
        std::vector<TypeBinding>& bindings
    ) const
    {
        // Trim whitespace from both types
        std::string patternTrimmed = pattern;
        std::string concreteTrimmed = concrete;

        // Remove leading/trailing whitespace
        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t"));
            s.erase(s.find_last_not_of(" \t") + 1);
        };
        trim(patternTrimmed);
        trim(concreteTrimmed);

        // Base case 1: Pattern is a type parameter (e.g., "T")
        if (isTypeParameter(patternTrimmed, declaredTypeParams))
        {
            // Bind this type parameter to the concrete type
            bindings.push_back(TypeBinding(patternTrimmed, concreteTrimmed));
            return true;
        }

        // Base case 2: Both are non-generic types (exact match required)
        std::string patternBase = substitutionService.extractBaseTypeName(patternTrimmed);
        std::string concreteBase = substitutionService.extractBaseTypeName(concreteTrimmed);

        std::vector<std::string> patternArgs = substitutionService.extractTypeArguments(patternTrimmed);
        std::vector<std::string> concreteArgs = substitutionService.extractTypeArguments(concreteTrimmed);

        // Base type names must match
        if (patternBase != concreteBase)
        {
            return false;  // Different base types - no match
        }

        // If no generic arguments, types match
        if (patternArgs.empty() && concreteArgs.empty())
        {
            return true;
        }

        // Generic argument counts must match
        if (patternArgs.size() != concreteArgs.size())
        {
            return false;  // Different number of type arguments - no match
        }

        // Recursively match each type argument
        for (size_t i = 0; i < patternArgs.size(); ++i)
        {
            if (!matchPatternRecursive(patternArgs[i], concreteArgs[i], declaredTypeParams, bindings))
            {
                return false;  // Nested argument doesn't match
            }
        }

        return true;
    }

    void GenericPatternAnalyzer::mergeBindings(
        std::unordered_map<std::string, std::string>& existingBindings,
        const std::vector<TypeBinding>& newBindings,
        const std::string& contextInfo
    ) const
    {
        for (const auto& binding : newBindings)
        {
            auto it = existingBindings.find(binding.parameterName);

            if (it == existingBindings.end())
            {
                // First binding for this parameter
                existingBindings[binding.parameterName] = binding.concreteType;
            }
            else if (it->second != binding.concreteType)
            {
                // Conflicting binding detected
                throw errors::TypeException(
                    "Type inference conflict for parameter '" + binding.parameterName + "': " +
                    "previously inferred as '" + it->second + "', " +
                    "but " + contextInfo + " suggests '" + binding.concreteType + "'"
                );
            }
            // else: same binding, no conflict
        }
    }

    std::unordered_map<std::string, std::string> GenericPatternAnalyzer::inferTypeBindings(
        const std::vector<std::string>& parameterPatterns,
        const std::vector<std::string>& argumentTypes,
        const std::unordered_set<std::string>& declaredTypeParams
    ) const
    {
        std::unordered_map<std::string, std::string> bindings;

        // Match each parameter pattern against its corresponding argument type
        size_t matchCount = std::min(parameterPatterns.size(), argumentTypes.size());

        for (size_t i = 0; i < matchCount; ++i)
        {
            const std::string& pattern = parameterPatterns[i];
            const std::string& concrete = argumentTypes[i];

            // Perform pattern matching
            std::vector<TypeBinding> paramBindings = matchPattern(pattern, concrete, declaredTypeParams);

            // Merge bindings, checking for conflicts
            std::string contextInfo = "argument " + std::to_string(i + 1) +
                                     " (pattern: " + pattern + ", actual: " + concrete + ")";
            mergeBindings(bindings, paramBindings, contextInfo);
        }

        return bindings;
    }
}
