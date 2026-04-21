#pragma once

#include <string>
#include <unordered_set>

namespace ast { class ASTNode; }

namespace vm::compiler::analysis
{
    /**
     * Phase 2 (MYT allocation perf): compute the set of `this.<field>` names
     * that a constructor definitely assigns before any possible read.
     *
     * Only these fields are safe to skip default-initialising at NEW_OBJECT
     * time — the constructor is guaranteed to overwrite them before anything
     * observes the slot.
     *
     * The analysis is intentionally very conservative: it recognises the
     * "trivial straight-line constructor" shape only (a BlockNode of
     * `this.F = pure_expr` assignments, where the RHS is a literal /
     * parameter read / arithmetic on same). Any branch, loop, method call,
     * super()/this() call, or anything that could re-read `this.X` causes
     * the analyser to return the empty set (safe fallback — default-init
     * still runs for every field).
     *
     * The conservative fallback is correctness-critical: a false positive
     * here lets the constructor read a garbage slot.
     */
    class DefiniteAssignmentAnalyzer
    {
    public:
        static std::unordered_set<std::string> analyzeConstructor(ast::ASTNode* body);
    };
}
