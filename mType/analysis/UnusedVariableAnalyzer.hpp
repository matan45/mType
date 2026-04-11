#pragma once

#include <vector>
#include "../diagnostics/Diagnostic.hpp"

namespace ast
{
    class ASTNode;
}

namespace analysis
{
    /**
     * Detects local variables that are declared but never read, and emits
     * one `MT-W2001` warning per occurrence (with two quick-fix
     * suggestions: rename to `_name` or remove the declaration).
     *
     * This is a **conservative** v1 analyzer:
     *   - Function/lambda parameters are NOT tracked.
     *   - For-each / catch / for(let i = 0; …) loop variables are NOT tracked.
     *   - When the walker encounters an AST node it doesn't know how to
     *     traverse, it marks every declaration currently in scope as used
     *     and stops emitting warnings for that scope. This guarantees we
     *     never produce a false positive — at the cost of false negatives
     *     in code paths that use rare constructs.
     *   - Underscore-prefixed names (`_foo`) are treated as a manual
     *     "intentionally unused" signal and skipped.
     *
     * The analyzer is invoked once per program from
     * `ScriptInterpreter::executeScriptAST` (CLI path) and from
     * `DocumentManager::parseDocument` (LSP path). Both call sites push
     * the resulting diagnostics into the warning sink they own
     * (`BytecodeCompiler::warnings_` for the CLI, `Document::diagnostics`
     * for the LSP).
     */
    class UnusedVariableAnalyzer
    {
    public:
        // Walks `program` once and returns one Diagnostic per detected
        // unused local. Does not mutate the AST.
        static std::vector<diagnostics::Diagnostic> analyze(ast::ASTNode* program);
    };
}
