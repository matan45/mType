#pragma once

#include <memory>
#include <string>
#include <vector>

// Forward declarations to keep this header light. The .cpp pulls in
// the full Environment / Scope / Registry headers from mtype-core.
namespace environment
{
    class Environment;
}

namespace diagnostics
{
    /**
     * Walks an Environment + current Scope and returns the deduplicated,
     * sorted list of identifier names visible at the failure site.
     *
     * Used by throwers to capture an "identifier pool" snapshot that
     * gets attached to UndefinedException / ClassNotFoundException etc.
     * The diagnostic converter then runs Levenshtein over the pool to
     * power "did you mean ...?" suggestions.
     *
     * Always returns a sorted vector (stable diagnostics across runs).
     * Returns an empty vector if the environment is null — callers can
     * pass the result through unconditionally.
     *
     * The pool intentionally combines variables, functions, and classes
     * so a single typo on (e.g.) `printl` can suggest the function
     * `println`. Per-context filters (variables only / classes only)
     * are exposed as separate functions when the dispatch site has
     * narrower context.
     */
    class IdentifierEnumerator
    {
    public:
        /**
         * Every visible name (variables in the current scope chain,
         * functions, classes). Default for symbol-not-found errors.
         */
        static std::vector<std::string> visibleIdentifiers(
            const environment::Environment* env);

        /// Variables only — visible from the current scope upward.
        static std::vector<std::string> visibleVariables(
            const environment::Environment* env);

        /// Top-level functions only.
        static std::vector<std::string> visibleFunctions(
            const environment::Environment* env);

        /// Top-level classes only.
        static std::vector<std::string> visibleClasses(
            const environment::Environment* env);

        /// Top-level interfaces only.
        static std::vector<std::string> visibleInterfaces(
            const environment::Environment* env);
    };
}
