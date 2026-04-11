#include "DiagnosticsTestSuite.hpp"

#include "../../diagnostics/SourceFileCache.hpp"
#include "../../util/DidYouMean.hpp"
#include "../../util/EditDistance.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace tests::testSuite
{
    using namespace testFramework;
    using services::ScriptAPI;

    namespace
    {
        // Throw on failure — the NATIVE_CALLBACK runner converts this into
        // a FAILED test with the message attached.
        void require(bool cond, const std::string& msg)
        {
            if (!cond)
            {
                throw std::runtime_error(msg);
            }
        }
    }

    void DiagnosticsTestSuite::setupTests()
    {
        // ================================================================
        // util::levenshtein
        // ================================================================

        addCallbackTest("levenshtein: identical strings have distance 0", "",
            [](ScriptAPI&) {
                require(util::levenshtein("foo", "foo", 5) == 0,
                    "expected 0 for identical strings");
            });

        addCallbackTest("levenshtein: empty vs N has distance N", "",
            [](ScriptAPI&) {
                require(util::levenshtein("", "abc", 5) == 3, "empty -> 'abc' should be 3");
                require(util::levenshtein("abc", "", 5) == 3, "'abc' -> empty should be 3");
                require(util::levenshtein("", "", 5) == 0, "empty -> empty should be 0");
            });

        addCallbackTest("levenshtein: single substitution", "",
            [](ScriptAPI&) {
                require(util::levenshtein("foo", "fou", 5) == 1,
                    "foo -> fou should be 1 substitution");
            });

        addCallbackTest("levenshtein: classic kitten/sitting = 3", "",
            [](ScriptAPI&) {
                require(util::levenshtein("kitten", "sitting", 5) == 3,
                    "kitten -> sitting is the textbook distance 3");
            });

        addCallbackTest("levenshtein: respects maxDistance early-out", "",
            [](ScriptAPI&) {
                // 7 edits required, but we only ask for budget 2 — must
                // return budget+1 == 3 (not the true distance 7).
                const int result = util::levenshtein("abcdefg", "1234567", 2);
                require(result == 3,
                    "expected early-out to return maxDistance+1 == 3, got "
                    + std::to_string(result));
            });

        addCallbackTest("levenshtein: length-difference quick reject", "",
            [](ScriptAPI&) {
                // |m - n| = 5 > maxDistance 2, must short-circuit.
                const int result = util::levenshtein("a", "abcdef", 2);
                require(result == 3,
                    "expected length-difference reject (3), got "
                    + std::to_string(result));
            });

        // ================================================================
        // util::findClosestMatch
        // ================================================================

        addCallbackTest("didYouMean: returns nullopt for empty pool", "",
            [](ScriptAPI&) {
                std::vector<std::string> empty;
                auto result = util::findClosestMatch("foo", empty);
                require(!result.has_value(), "expected nullopt for empty pool");
            });

        addCallbackTest("didYouMean: finds typo within budget", "",
            [](ScriptAPI&) {
                const std::vector<std::string> pool = { "foo", "bar", "baz" };
                auto result = util::findClosestMatch("foa", pool);
                require(result.has_value(), "expected a match for 'foa'");
                require(*result == "foo",
                    "expected 'foo', got '" + *result + "'");
            });

        addCallbackTest("didYouMean: skips exact matches", "",
            [](ScriptAPI&) {
                // 'foo' is in the pool and matches exactly — must skip
                // and return the next-best (none, since 'bar' is too far).
                const std::vector<std::string> pool = { "foo", "bar" };
                auto result = util::findClosestMatch("foo", pool);
                require(!result.has_value(),
                    "expected nullopt when query exactly matches a candidate");
            });

        addCallbackTest("didYouMean: respects rustc heuristic budget", "",
            [](ScriptAPI&) {
                // Query length 3 → budget = max(1, 3/3) = 1.
                // 'aaa' vs 'bbb' is distance 3, must reject.
                const std::vector<std::string> pool = { "bbb" };
                auto result = util::findClosestMatch("aaa", pool);
                require(!result.has_value(),
                    "expected nullopt when distance exceeds rustc budget");
            });

        addCallbackTest("didYouMean: tie-break is alphabetic", "",
            [](ScriptAPI&) {
                // Both 'foa' and 'foc' are distance 1 from 'fob' — should
                // pick 'foa' deterministically.
                const std::vector<std::string> pool = { "foc", "foa" };
                auto result = util::findClosestMatch("fob", pool);
                require(result.has_value(), "expected a match");
                require(*result == "foa",
                    "expected alphabetic tie-break 'foa', got '" + *result + "'");
            });

        addCallbackTest("didYouMean: handles longer identifiers", "",
            [](ScriptAPI&) {
                // length 12 → budget = max(1, 4) = 4 (capped at default 3).
                // Effective budget = min(3, 4) = 3 — distance 1 fits.
                const std::vector<std::string> pool = { "calculateSum", "totalize" };
                auto result = util::findClosestMatch("calculateSun", pool);
                require(result.has_value(), "expected a match for one-letter typo");
                require(*result == "calculateSum",
                    "expected 'calculateSum', got '" + *result + "'");
            });

        // ================================================================
        // diagnostics::SourceFileCache
        // ================================================================

        addCallbackTest("SourceFileCache: publish + getLine round-trip", "",
            [](ScriptAPI&) {
                auto& cache = diagnostics::SourceFileCache::instance();
                cache.invalidate("test/cache_round_trip.mt");
                cache.publishFromContent(
                    "test/cache_round_trip.mt",
                    "first line\nsecond line\nthird line");

                auto line1 = cache.getLine("test/cache_round_trip.mt", 1);
                auto line2 = cache.getLine("test/cache_round_trip.mt", 2);
                auto line3 = cache.getLine("test/cache_round_trip.mt", 3);
                auto line4 = cache.getLine("test/cache_round_trip.mt", 4);

                require(line1.has_value() && *line1 == "first line",
                    "line 1 missing or wrong");
                require(line2.has_value() && *line2 == "second line",
                    "line 2 missing or wrong");
                require(line3.has_value() && *line3 == "third line",
                    "line 3 missing or wrong");
                require(!line4.has_value(),
                    "out-of-range line should be nullopt");

                cache.invalidate("test/cache_round_trip.mt");
            });

        addCallbackTest("SourceFileCache: handles CRLF line endings", "",
            [](ScriptAPI&) {
                auto& cache = diagnostics::SourceFileCache::instance();
                cache.invalidate("test/cache_crlf.mt");
                cache.publishFromContent(
                    "test/cache_crlf.mt",
                    "alpha\r\nbeta\r\ngamma");

                auto line1 = cache.getLine("test/cache_crlf.mt", 1);
                auto line2 = cache.getLine("test/cache_crlf.mt", 2);

                require(line1.has_value() && *line1 == "alpha",
                    "CRLF line 1 should strip the carriage return");
                require(line2.has_value() && *line2 == "beta",
                    "CRLF line 2 should strip the carriage return");

                cache.invalidate("test/cache_crlf.mt");
            });

        addCallbackTest("SourceFileCache: invalidate drops the file", "",
            [](ScriptAPI&) {
                auto& cache = diagnostics::SourceFileCache::instance();
                cache.publishFromContent("test/cache_invalidate.mt", "only line");
                require(cache.getLineCount("test/cache_invalidate.mt") == 1,
                    "expected 1 line before invalidate");
                cache.invalidate("test/cache_invalidate.mt");
                require(cache.getLineCount("test/cache_invalidate.mt") == 0,
                    "expected 0 lines after invalidate");
            });

        addCallbackTest("SourceFileCache: filename normalization", "",
            [](ScriptAPI&) {
                auto& cache = diagnostics::SourceFileCache::instance();
                cache.invalidate("foo/bar/baz.mt");
                cache.publishFromContent("foo/bar/baz.mt", "content");

                // Backslashes and forward slashes should resolve to the
                // same cache entry. Double separators should also collapse.
                auto a = cache.getLine("foo\\bar\\baz.mt", 1);
                auto b = cache.getLine("foo//bar/baz.mt", 1);
                auto c = cache.getLine("foo/bar/baz.mt", 1);

                require(a.has_value() && *a == "content",
                    "backslash variant should hit the same entry");
                require(b.has_value() && *b == "content",
                    "double-slash variant should hit the same entry");
                require(c.has_value() && *c == "content",
                    "canonical form should hit");

                cache.invalidate("foo/bar/baz.mt");
            });

        addCallbackTest("SourceFileCache: getLine rejects line < 1", "",
            [](ScriptAPI&) {
                auto& cache = diagnostics::SourceFileCache::instance();
                cache.publishFromContent("test/cache_zero.mt", "x");
                auto result = cache.getLine("test/cache_zero.mt", 0);
                require(!result.has_value(), "line 0 should be rejected");
                cache.invalidate("test/cache_zero.mt");
            });

        addCallbackTest("SourceFileCache: normalizeFilename collapses separators", "",
            [](ScriptAPI&) {
                using diagnostics::SourceFileCache;
                require(SourceFileCache::normalizeFilename("a\\b\\c") == "a/b/c",
                    "backslashes should become forward slashes");
                require(SourceFileCache::normalizeFilename("a//b///c") == "a/b/c",
                    "consecutive slashes should collapse");
                require(SourceFileCache::normalizeFilename("a/b/c") == "a/b/c",
                    "already-normalized form should pass through");
            });
    }
}
