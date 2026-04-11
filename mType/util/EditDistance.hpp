#pragma once
#include <string_view>

namespace util
{
    /**
     * Compute the Levenshtein edit distance between two strings.
     *
     * Uses the standard two-row dynamic-programming formulation. Returns
     * `maxDistance + 1` (rather than the true distance) once the lower
     * bound on every cell in the current row exceeds `maxDistance`, so
     * the caller can pass a small bound for cheap rejection.
     *
     * @param a            First string.
     * @param b            Second string.
     * @param maxDistance  Caller-supplied upper bound; pass a generous
     *                     value (e.g., max(a.size(), b.size())) to compute
     *                     the true distance.
     */
    int levenshtein(std::string_view a, std::string_view b, int maxDistance);
}
