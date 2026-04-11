#include "EditDistance.hpp"
#include <algorithm>
#include <vector>

namespace util
{
    int levenshtein(std::string_view a, std::string_view b, int maxDistance)
    {
        const int m = static_cast<int>(a.size());
        const int n = static_cast<int>(b.size());

        // Quick rejection: if the length difference alone exceeds the
        // budget, no edit script can fit.
        if (std::abs(m - n) > maxDistance)
        {
            return maxDistance + 1;
        }

        if (m == 0) return n <= maxDistance ? n : maxDistance + 1;
        if (n == 0) return m <= maxDistance ? m : maxDistance + 1;

        // Two rolling rows.
        std::vector<int> prev(n + 1);
        std::vector<int> curr(n + 1);

        for (int j = 0; j <= n; ++j)
        {
            prev[j] = j;
        }

        for (int i = 1; i <= m; ++i)
        {
            curr[0] = i;
            int rowMin = curr[0];
            for (int j = 1; j <= n; ++j)
            {
                const int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
                curr[j] = std::min({
                    prev[j] + 1,           // deletion
                    curr[j - 1] + 1,       // insertion
                    prev[j - 1] + cost     // substitution
                });
                rowMin = std::min(rowMin, curr[j]);
            }
            // Early-out: every cell in this row already exceeds the budget,
            // so the final cell can never be ≤ maxDistance.
            if (rowMin > maxDistance)
            {
                return maxDistance + 1;
            }
            std::swap(prev, curr);
        }

        const int result = prev[n];
        return (result <= maxDistance) ? result : maxDistance + 1;
    }
}
