#include "OverloadResolver.hpp"
#include <cstddef>
#include "../../../environment/registry/TypeCatalog.hpp"
#include <climits>

namespace vm::compiler::overload
{
    template<typename T>
    CandidateMatch OverloadResolver::matchCandidate(
        const std::shared_ptr<T>& candidate,
        const std::vector<value::ParameterType>& argumentTypes,
        const environment::registry::TypeCatalog& catalog)
    {
        CandidateMatch match;
        const auto& parameters = getParameters(candidate);

        if (parameters.size() != argumentTypes.size()) {
            match.totalScore = INT_MAX;
            return match;
        }

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
            // Viable = valid conversions AND parameter count matched (totalScore != INT_MAX).
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

        std::vector<std::pair<std::shared_ptr<T>, CandidateMatch>> bestMatches;
        bestMatches.push_back(viableMatches[0]);

        for (size_t i = 1; i < viableMatches.size(); ++i) {
            const auto& current = viableMatches[i];
            const auto& best = bestMatches[0];

            if (current.second.isBetterThan(best.second)) {
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

        if (bestMatches.size() > 1) {
            result.isAmbiguous = true;
            for (const auto& match : bestMatches) {
                result.ambiguousCandidates.push_back(match.first);
            }
        } else {
            result.isAmbiguous = false;
            result.selectedOverload = bestMatches[0].first;
        }

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
