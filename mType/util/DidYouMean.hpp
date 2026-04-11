#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace util
{
    /**
     * Find the candidate identifier closest to a misspelled query.
     *
     * Uses Levenshtein edit distance with rustc's heuristic: only consider
     * candidates within `max(1, query.size() / 3)` edits, capped at
     * `maxDistance`. The closest candidate is returned. Ties are broken by
     * alphabetic order so the result is deterministic.
     *
     * Returns `std::nullopt` if no candidate is close enough.
     *
     * @param query        The misspelled identifier the user typed.
     * @param candidates   Pool of valid identifiers visible at the error site.
     * @param maxDistance  Hard upper bound on edit distance (default 3).
     */
    std::optional<std::string> findClosestMatch(
        std::string_view query,
        const std::vector<std::string>& candidates,
        int maxDistance = 3);
}
