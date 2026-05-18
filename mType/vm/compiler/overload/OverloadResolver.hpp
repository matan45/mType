#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>
#include "../../../value/ParameterType.hpp"
#include "../../../value/ValueType.hpp"
#include "../../../environment/registry/MethodDefinition.hpp"
#include "../../../environment/registry/FunctionDefinition.hpp"
#include "../../../errors/SourceLocation.hpp"

namespace environment::registry { class TypeCatalog; }

namespace vm::compiler::overload
{
    /**
     * Enumeration of conversion types, ordered by preference
     * Lower values indicate better/more specific matches
     */
    enum class ConversionType
    {
        EXACT_MATCH = 0,           // Perfect type match
        NUMERIC_WIDENING = 1,      // int -> float
        AUTO_BOXING = 2,           // int -> Int, float -> Float
        INHERITANCE = 3,           // Child -> Parent class
        GENERIC_PARAMETER = 10,    // Match to generic type parameter (T, K, etc.) - lower priority than specific types
        INCOMPATIBLE = 999         // No valid conversion
    };

    /**
     * Result of conversion analysis for a single parameter
     */
    struct ParameterConversion
    {
        ConversionType conversionType;
        int inheritanceDistance;   // Number of steps in inheritance chain (0 for exact match)

        ParameterConversion(ConversionType type = ConversionType::INCOMPATIBLE, int distance = 0)
            : conversionType(type), inheritanceDistance(distance) {}

        // Check if conversion is valid
        bool isValid() const { return conversionType != ConversionType::INCOMPATIBLE; }

        // Comparison for ranking (lower is better)
        bool isBetterThan(const ParameterConversion& other) const;
    };

    /**
     * Result of matching a candidate against argument types
     */
    struct CandidateMatch
    {
        std::vector<ParameterConversion> conversions;
        int totalScore;  // Lower is better

        CandidateMatch() : totalScore(INT_MAX) {}

        // Check if all parameters can be converted
        bool isViable() const;

        // Calculate total score based on conversions
        void calculateScore();

        // Comparison for ranking
        bool isBetterThan(const CandidateMatch& other) const;

        // Check if equally good (ambiguous)
        bool isEquivalentTo(const CandidateMatch& other) const;
    };

    /**
     * Result of overload resolution
     */
    template<typename T>
    struct OverloadResolutionResult
    {
        std::shared_ptr<T> selectedOverload;
        std::vector<std::shared_ptr<T>> ambiguousCandidates;  // Empty if not ambiguous
        std::vector<std::shared_ptr<T>> viableCandidates;     // All viable but not best
        bool isAmbiguous;
        bool hasViableCandidates;

        OverloadResolutionResult()
            : selectedOverload(nullptr), isAmbiguous(false), hasViableCandidates(false) {}
    };

    /**
     * Service for resolving method and function overloads
     * Implements best match algorithm with conversion ranking
     */
    class OverloadResolver
    {
    public:
        /**
         * Resolve method overload from candidates
         *
         * @param candidates Vector of method candidates with same name
         * @param argumentTypes Actual argument types at call site
         * @param sourceLocation Source location for error reporting
         * @return Resolution result with selected overload or ambiguity information
         */
        static OverloadResolutionResult<runtimeTypes::klass::MethodDefinition> resolveMethodOverload(
            const std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>>& candidates,
            const std::vector<value::ParameterType>& argumentTypes,
            const ast::SourceLocation& sourceLocation,
            const environment::registry::TypeCatalog& catalog);

        /**
         * Resolve method overload using argument type names
         */
        static OverloadResolutionResult<runtimeTypes::klass::MethodDefinition> resolveMethodOverload(
            const std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>>& candidates,
            const std::vector<std::string>& argumentTypeNames,
            const ast::SourceLocation& sourceLocation,
            const environment::registry::TypeCatalog& catalog);

        /**
         * Resolve function overload from candidates
         */
        static OverloadResolutionResult<runtimeTypes::global::FunctionDefinition> resolveFunctionOverload(
            const std::vector<std::shared_ptr<runtimeTypes::global::FunctionDefinition>>& candidates,
            const std::vector<value::ParameterType>& argumentTypes,
            const ast::SourceLocation& sourceLocation,
            const environment::registry::TypeCatalog& catalog);

        /**
         * Resolve function overload using argument type names
         */
        static OverloadResolutionResult<runtimeTypes::global::FunctionDefinition> resolveFunctionOverload(
            const std::vector<std::shared_ptr<runtimeTypes::global::FunctionDefinition>>& candidates,
            const std::vector<std::string>& argumentTypeNames,
            const ast::SourceLocation& sourceLocation,
            const environment::registry::TypeCatalog& catalog);

        /**
         * Analyze conversion required for a single parameter
         */
        static ParameterConversion analyzeParameterConversion(
            const value::ParameterType& argumentType,
            const value::ParameterType& parameterType,
            const environment::registry::TypeCatalog& catalog);

        /**
         * Analyze conversion using type names
         */
        static ParameterConversion analyzeParameterConversion(
            const std::string& argumentTypeName,
            const std::string& parameterTypeName,
            const environment::registry::TypeCatalog& catalog);

        /**
         * Check if one type can be converted to another
         */
        static ConversionType getConversionType(
            const value::ParameterType& from,
            const value::ParameterType& to,
            const environment::registry::TypeCatalog& catalog);

        /**
         * Calculate inheritance distance between two class types
         */
        static int calculateInheritanceDistance(
            const std::string& childType,
            const std::string& parentType,
            const environment::registry::TypeCatalog& catalog);

    private:
        /**
         * Match a candidate definition against argument types
         */
        template<typename T>
        static CandidateMatch matchCandidate(
            const std::shared_ptr<T>& candidate,
            const std::vector<value::ParameterType>& argumentTypes,
            const environment::registry::TypeCatalog& catalog);

        /**
         * Filter viable candidates (those that can accept the arguments)
         */
        template<typename T>
        static std::vector<std::pair<std::shared_ptr<T>, CandidateMatch>> filterViableCandidates(
            const std::vector<std::shared_ptr<T>>& candidates,
            const std::vector<value::ParameterType>& argumentTypes,
            const environment::registry::TypeCatalog& catalog);

        /**
         * Select best candidate from viable matches
         */
        template<typename T>
        static OverloadResolutionResult<T> selectBestCandidate(
            const std::vector<std::pair<std::shared_ptr<T>, CandidateMatch>>& viableMatches);

        /**
         * Convert type name string to ParameterType
         */
        static value::ParameterType typeNameToParameterType(const std::string& typeName);

        /**
         * Get parameters from MethodDefinition (skips 'this' for instance methods)
         */
        static std::vector<std::pair<std::string, value::ParameterType>> getParameters(
            const std::shared_ptr<runtimeTypes::klass::MethodDefinition>& method);

        /**
         * Get parameters from FunctionDefinition
         */
        static const std::vector<std::pair<std::string, value::ParameterType>>& getParameters(
            const std::shared_ptr<runtimeTypes::global::FunctionDefinition>& function);
    };

} // namespace vm::compiler::overload
