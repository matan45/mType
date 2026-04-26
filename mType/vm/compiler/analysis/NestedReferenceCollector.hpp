#pragma once

#include <string>
#include <unordered_set>

namespace ast { class ASTNode; }

namespace vm::compiler::analysis
{
    // Result of a NestedReferenceCollector::collect run. `pessimistic == true`
    // means the walker hit an AST node it didn't know how to traverse, so it
    // bailed out before finishing. Callers MUST check `pessimistic` before
    // consulting `names` — when set, treat every top-level name as referenced
    // and disable promotion.
    struct NestedReferenceResult
    {
        std::unordered_set<std::string> names;
        bool pessimistic = false;
    };

    /**
     * MYT-XXX (top-level decl promotion): walks the entire program AST and
     * collects every identifier name that appears in the body of a nested
     * non-lambda function or method declaration. Lambdas are explicitly
     * excluded — they capture outer scope via the lambda-capture mechanism,
     * which works regardless of whether the captured name is a global or a
     * top-level local.
     *
     * The collector is INTENTIONALLY conservative: it does not track scope.
     * If a nested function declares a local `x` and a top-level `x` also
     * exists, the top-level `x` is reported as "referenced by nested
     * function" even though the local shadows it. This gives a false-
     * negative on the promotion path (the top-level `x` stays a global)
     * but never a false positive (we never promote a name that's actually
     * needed as a global by a nested function).
     *
     * Used by StatementCompiler::canPromoteTopLevel to decide whether a
     * top-level `int N = 10000000;` is safe to emit as STORE_LOCAL into the
     * entry frame's slot vs DECLARE_VAR into the runtime Environment.
     */
    class NestedReferenceCollector
    {
    public:
        // Walk `root` (typically a ProgramNode). Returns the set of
        // identifier names referenced from any nested non-lambda function
        // or method body anywhere in the tree, plus a pessimistic flag set
        // when the walker bailed out on an unknown node type.
        static NestedReferenceResult collect(ast::ASTNode* root);
    };
}
