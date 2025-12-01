#pragma once

#include "UnifiedType.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace ast
{
    struct GenericTypeParameter;
}

namespace types
{
    /**
     * Centralized service for type substitution operations.
     * Handles all generic type parameter replacement across parser, compiler, and runtime.
     *
     * Replaces:
     * - GenericType::substitute() internal logic
     * - Legacy string-based substitution code
     * - Scattered substitution code in ClassRegistrar
     *
     * Features:
     * - Circular dependency detection
     * - Multi-level inheritance chain composition
     * - Type argument inference from usage context
     */
    class TypeSubstitutionService
    {
    public:
        TypeSubstitutionService() = default;
        ~TypeSubstitutionService() = default;

        // Non-copyable, non-movable (stateless service)
        TypeSubstitutionService(const TypeSubstitutionService&) = delete;
        TypeSubstitutionService& operator=(const TypeSubstitutionService&) = delete;

        /**
         * Applies type parameter substitutions to a type.
         *
         * @param type The type to substitute in (may contain generic parameters)
         * @param substitutions Map from parameter names to concrete types
         * @return New type with all substitutions applied
         * @throws std::runtime_error on circular substitution
         *
         * Example:
         *   substitute(Container<T>, {T: String}) -> Container<String>
         *   substitute(Map<K, V>, {K: String, V: Int}) -> Map<String, Int>
         */
        UnifiedTypePtr substitute(
            const UnifiedTypePtr& type,
            const TypeSubstitutionMap& substitutions) const;

        /**
         * Builds a substitution map from generic parameters and concrete arguments.
         *
         * @param parameters Generic type parameters from class/method definition
         * @param arguments Concrete type arguments from instantiation
         * @return Map from parameter names to concrete types
         * @throws std::invalid_argument if counts don't match
         *
         * Example:
         *   parameters: [T, U]
         *   arguments: [String, Int]
         *   result: {T: String, U: Int}
         */
        TypeSubstitutionMap buildSubstitutionMap(
            const std::vector<ast::GenericTypeParameter>& parameters,
            const std::vector<UnifiedTypePtr>& arguments) const;

        /**
         * Overload using parameter names directly.
         */
        TypeSubstitutionMap buildSubstitutionMap(
            const std::vector<std::string>& parameterNames,
            const std::vector<UnifiedTypePtr>& arguments) const;

        /**
         * Composes two substitution maps for multi-level inheritance.
         *
         * Given:
         *   outer: {T: U}     (from Parent<T> extends GrandParent<T>)
         *   inner: {U: String} (from Child extends Parent<String>)
         *
         * Returns: {T: String, U: String}
         *
         * The outer substitutions are first applied, then the inner.
         */
        TypeSubstitutionMap composeSubstitutions(
            const TypeSubstitutionMap& outer,
            const TypeSubstitutionMap& inner) const;

        /**
         * Composes a chain of substitution maps for deep inheritance.
         *
         * @param chain Substitution maps ordered from root to leaf
         * @return Single composed substitution map
         *
         * Example for: GrandChild -> Child -> Parent -> GrandParent
         *   chain[0]: GrandParent's params to Parent's args
         *   chain[1]: Parent's params to Child's args
         *   chain[2]: Child's params to GrandChild's args
         */
        TypeSubstitutionMap composeChain(
            const std::vector<TypeSubstitutionMap>& chain) const;

        /**
         * Infers type parameter bindings from argument types.
         *
         * Given a method signature and actual argument types, determines
         * what the type parameters should be bound to.
         *
         * @param parameterTypes Expected parameter types (may contain generic params)
         * @param argumentTypes Actual argument types (concrete)
         * @param typeParamNames Names of type parameters to infer
         * @return Inferred substitution map
         * @throws std::runtime_error on conflicting inferences
         *
         * Example:
         *   parameterTypes: [Box<T>, T]
         *   argumentTypes: [Box<String>, String]
         *   typeParamNames: {T}
         *   result: {T: String}
         */
        TypeSubstitutionMap inferFromArguments(
            const std::vector<UnifiedTypePtr>& parameterTypes,
            const std::vector<UnifiedTypePtr>& argumentTypes,
            const std::unordered_set<std::string>& typeParamNames) const;

        /**
         * Extracts all generic parameter names from a type.
         *
         * @param type Type to analyze
         * @return Set of all generic parameter names found
         *
         * Example:
         *   Map<K, List<V>> -> {K, V}
         */
        std::unordered_set<std::string> extractGenericParameters(
            const UnifiedTypePtr& type) const;

        /**
         * Checks if a substitution would result in a valid type.
         * Validates that all referenced types exist and constraints are satisfied.
         *
         * @param substitutions The substitution map to validate
         * @return true if all substitutions are valid
         */
        bool validateSubstitutions(const TypeSubstitutionMap& substitutions) const;

        /**
         * Resolves a type in an inheritance context.
         *
         * Given a type from a parent class and the inheritance substitution chain,
         * fully resolves the type for the child class context.
         *
         * @param type Type from ancestor class
         * @param inheritanceChain Substitution maps from root to current class
         * @return Fully resolved type
         */
        UnifiedTypePtr resolveInInheritanceContext(
            const UnifiedTypePtr& type,
            const std::vector<TypeSubstitutionMap>& inheritanceChain) const;

        // ============== String-based helper methods ==============
        // These provide compatibility with existing code that uses string-based types.

        /**
         * Parses a generic type name into base name and type arguments.
         * Example: "Container<String, Int>" -> ("Container", ["String", "Int"])
         */
        std::pair<std::string, std::vector<std::string>> parseGenericTypeName(
            const std::string& typeName) const;

        /**
         * Resolves a type string with string-based substitutions.
         * Example: resolveGenericType("List<T>", {T: "String"}) -> "List<String>"
         */
        std::string resolveGenericType(
            const std::string& typeName,
            const std::unordered_map<std::string, std::string>& substitutions) const;

        /**
         * Extracts the base type name without generic parameters.
         * Example: "List<Int>" -> "List"
         */
        std::string extractBaseTypeName(const std::string& typeName) const;

        /**
         * Extracts type arguments from a generic type string.
         * Example: "Map<String, Int>" -> ["String", "Int"]
         */
        std::vector<std::string> extractTypeArguments(const std::string& typeName) const;

    private:
        static constexpr int MAX_SUBSTITUTION_DEPTH = 50;
        static constexpr int MAX_INFERENCE_ITERATIONS = 100;

        /**
         * Internal inference helper that matches a pattern type against a concrete type.
         */
        void inferFromSingleArgument(
            const UnifiedTypePtr& patternType,
            const UnifiedTypePtr& concreteType,
            const std::unordered_set<std::string>& typeParamNames,
            TypeSubstitutionMap& inferred,
            std::unordered_set<std::string>& conflicts) const;

        /**
         * Collects generic parameters recursively.
         */
        void collectGenericParameters(
            const UnifiedTypePtr& type,
            std::unordered_set<std::string>& params) const;
    };
}
