#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include "../../../types/TypeSubstitutionService.hpp"

namespace vm::compiler::types
{
    /**
     * Represents a binding discovered during pattern matching
     * Example: Matching Box<T> against Box<Int> produces binding T->Int
     */
    struct TypeBinding
    {
        std::string parameterName;  // e.g., "T", "K", "V"
        std::string concreteType;   // e.g., "Int", "String", "Box<Int>"

        TypeBinding(const std::string& param, const std::string& concrete)
            : parameterName(param), concreteType(concrete)
        {
        }
    };

    /**
     * Analyzes generic type patterns and performs pattern matching
     * to extract type parameter bindings from nested generic structures.
     *
     * Example use cases:
     * - Match Box<T> against Box<Int> to infer T=Int
     * - Match Map<K,V> against Map<String,Int> to infer K=String, V=Int
     * - Match List<Box<T>> against List<Box<Int>> to infer T=Int (nested)
     *
     * This enables type inference for functions like:
     *   function<T> processBox(Box<T> item): T
     * Called as:
     *   processBox(myBoxOfInt)  // Infers T=Int from Box<Int>
     */
    class GenericPatternAnalyzer
    {
    public:
        GenericPatternAnalyzer() = default;
        ~GenericPatternAnalyzer() = default;

        /**
         * Match a pattern type against a concrete type to extract bindings
         *
         * @param patternType The pattern with type parameters (e.g., "Box<T>")
         * @param concreteType The concrete type to match against (e.g., "Box<Int>")
         * @param declaredTypeParams The declared type parameters (e.g., ["T"])
         * @return Vector of type bindings extracted from the match
         *
         * Returns empty vector if match fails (incompatible types)
         */
        std::vector<TypeBinding> matchPattern(
            const std::string& patternType,
            const std::string& concreteType,
            const std::unordered_set<std::string>& declaredTypeParams
        ) const;

        /**
         * Infer type parameter bindings from multiple function arguments
         *
         * @param parameterPatterns Vector of parameter type patterns
         * @param argumentTypes Vector of concrete argument types
         * @param declaredTypeParams Set of declared type parameters
         * @return Map of type parameter names to their inferred concrete types
         *
         * Throws TypeException if:
         * - Conflicting bindings are found (T inferred as both Int and String)
         * - Pattern and concrete types are incompatible
         */
        std::unordered_map<std::string, std::string> inferTypeBindings(
            const std::vector<std::string>& parameterPatterns,
            const std::vector<std::string>& argumentTypes,
            const std::unordered_set<std::string>& declaredTypeParams
        ) const;

        /**
         * Check if a type name represents a generic type parameter
         * Examples: "T" -> true, "Box" -> false, "Box<T>" -> false
         */
        static bool isTypeParameter(
            const std::string& typeName,
            const std::unordered_set<std::string>& declaredTypeParams
        );

        /**
         * Extract all type parameters used in a type expression
         * Example: "Map<K, List<V>>" with params {"K","V"} -> {"K", "V"}
         */
        static std::unordered_set<std::string> extractUsedTypeParameters(
            const std::string& typeExpression,
            const std::unordered_set<std::string>& declaredTypeParams
        );

        static constexpr int MAX_EXTRACT_DEPTH = 50;

    private:
        // Reusable service for type string parsing (avoids per-call construction)
        ::types::TypeSubstitutionService substitutionService;

        /**
         * Recursive helper for pattern matching
         * Handles nested generic structures
         */
        bool matchPatternRecursive(
            const std::string& pattern,
            const std::string& concrete,
            const std::unordered_set<std::string>& declaredTypeParams,
            std::vector<TypeBinding>& bindings
        ) const;

        /**
         * Merge new bindings with existing ones, detecting conflicts
         * Throws TypeException if conflicting bindings are found
         */
        void mergeBindings(
            std::unordered_map<std::string, std::string>& existingBindings,
            const std::vector<TypeBinding>& newBindings,
            const std::string& contextInfo
        ) const;
    };
}
