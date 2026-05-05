#pragma once

#include "../base/OptimizationPass.hpp"
#include <cstddef>
#include "../../ast/NodeClassesDeclaration.hpp"
#include <memory>

namespace optimizer::passes
{
    /**
     * MYT-274 — Structural Equality Synthesis Pass
     *
     * For every non-abstract, non-value, non-generic user class that has at
     * least one declared instance field and does not declare its own
     * `hashCode()` and/or `equals(Object)`, this pass synthesizes a fast
     * bytecode body for the missing method(s). Synthesized bodies follow the
     * Effective Java pattern (h = 31*h + f.hashCode()) and an
     * `isClassOf`-guarded structural equals; ≤4-field classes fit under the
     * inliner's INLINE_SIZE_LIMIT = 32 op budget so HashMap/HashSet hot
     * paths inline them at MONO/POLY call sites.
     *
     * Replaces the slow string-based Object.hashCode native default for the
     * common case of user-class HashMap/HashSet keys. Same observable
     * structural-equality contract; hash bit values change (acceptable —
     * Java/C# explicitly do not guarantee bit stability across runs).
     *
     * Skip rules and parent-conflict detection are in StructuralEqualityPolicy.
     * Method-body construction is in StructuralEqualityCodegen.
     */
    class StructuralEqualitySynthesisPass : public base::OptimizationPass
    {
    private:
        size_t classesSynthesized = 0;
        size_t hashCodeSynthesized = 0;
        size_t equalsSynthesized = 0;

    public:
        StructuralEqualitySynthesisPass();

        std::unique_ptr<ast::ASTNode> optimize(
            std::unique_ptr<ast::ASTNode> node,
            base::OptimizationContext& context) override;

        std::string getDescription() const override;
        void reportMetrics(OptimizationResult& result) const override;
        void reset() override;
    };
}
