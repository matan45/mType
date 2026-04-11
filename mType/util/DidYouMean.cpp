#include "DidYouMean.hpp"
#include <algorithm>
#include "EditDistance.hpp"

namespace util
{
    std::optional<std::string> findClosestMatch(
        std::string_view query,
        const std::vector<std::string>& candidates,
        int maxDistance)
    {
        if (query.empty() || candidates.empty())
        {
            return std::nullopt;
        }

        // rustc-style heuristic: tolerate at most ~⅓ of the query length
        // in edits, but never more than the caller's hard cap. Always
        // permit at least one edit so single-character typos still match
        // very short identifiers.
        const int relativeBudget = std::max(1, static_cast<int>(query.size()) / 3);
        const int budget = std::min(maxDistance, relativeBudget);

        int bestDistance = budget + 1;
        const std::string* bestCandidate = nullptr;

        for (const auto& candidate : candidates)
        {
            // Skip the trivial "exact match" case — if the user typed it
            // correctly, the caller should not have asked for a suggestion.
            if (candidate == query)
            {
                continue;
            }
            const int distance = levenshtein(query, candidate, budget);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestCandidate = &candidate;
            }
            else if (distance == bestDistance && bestCandidate != nullptr
                     && candidate < *bestCandidate)
            {
                // Deterministic tie-break for stable diagnostics.
                bestCandidate = &candidate;
            }
        }

        if (bestCandidate == nullptr)
        {
            return std::nullopt;
        }
        return *bestCandidate;
    }
}
