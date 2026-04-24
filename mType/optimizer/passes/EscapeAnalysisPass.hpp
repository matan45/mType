#pragma once

#include "../base/OptimizationPass.hpp"
#include "../../ast/ASTNode.hpp"
#include <memory>

namespace optimizer::passes {

    /**
     * MYT-134: Escape Analysis Pass
     *
     * Identifies `new ClassName(...)` allocations whose result never escapes the
     * enclosing function / method / constructor / lambda body, and marks their
     * NewNode so the bytecode compiler emits NEW_STACK instead of NEW_OBJECT.
     *
     * Correctness is paramount: a missed escape would mean a use-after-free at
     * runtime, since the promoted allocation is released at frame teardown.
     * Any uncertainty defaults to "escaping" and the allocation stays on the heap.
     *
     * Algorithm (per function-like body):
     *   1. Collect candidates — locals whose initializer is a pure NewNode.
     *   2. Walk the body with context-aware escape detection:
     *        * ReturnNode value, ThrowNode value, MemberAssignment RHS,
     *          IndexAssignment RHS, call arguments (function/method/constructor),
     *          any LambdaNode body — all treated as ESCAPING contexts.
     *        * MemberAccess receiver and MethodCall receiver — SAFE contexts.
     *   3. Aliasing via union-find: `y = x` on locals unions their escape sets.
     *   4. For each candidate whose root is not marked escaped → setIsStackAllocated(true).
     *
     * Out of scope (follow-up tickets):
     *   - Interprocedural / inlined analysis
     *   - Stack promotion of arrays or strings
     *   - Async/coroutine frames (conservatively escape everything)
     */
    class EscapeAnalysisPass : public base::OptimizationPass {
    public:
        EscapeAnalysisPass();

        std::unique_ptr<ast::ASTNode> optimize(
            std::unique_ptr<ast::ASTNode> node,
            base::OptimizationContext& context
        ) override;

        std::string getDescription() const override;
        void reportMetrics(OptimizationResult& result) const override;
        void reset() override;

    private:
        size_t stackPromotions;
    };

} // namespace optimizer::passes
