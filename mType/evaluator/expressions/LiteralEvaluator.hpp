#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../ast/nodes/expressions/FloatNode.hpp"
#include "../../ast/nodes/expressions/StringNode.hpp"
#include "../../ast/nodes/expressions/BoolNode.hpp"
#include "../../ast/nodes/expressions/NullNode.hpp"

namespace evaluator {
namespace expressions {

    using namespace base;
    using namespace value;
    using namespace ast::nodes::expressions;

    /**
     * @brief Evaluates literal expression nodes (primitives)
     *
     * Responsibilities:
     * - Evaluate integer literals
     * - Evaluate float literals
     * - Evaluate string literals
     * - Evaluate boolean literals
     * - Evaluate null literals
     *
     * Design Principles:
     * - Single Responsibility: Only literal value extraction
     * - Stateless: No internal state, uses string pool from context
     * - Simple: Pure data extraction with no complex logic
     */
    class LiteralEvaluator {
    private:
        std::shared_ptr<EvaluationContext> context;

    public:
        explicit LiteralEvaluator(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx) {}

        /**
         * Evaluate an integer literal node
         */
        Value evaluateInteger(IntegerNode* node);

        /**
         * Evaluate a float literal node
         */
        Value evaluateFloat(FloatNode* node);

        /**
         * Evaluate a string literal node (uses string pool)
         */
        Value evaluateString(StringNode* node);

        /**
         * Evaluate a boolean literal node
         */
        Value evaluateBool(BoolNode* node);

        /**
         * Evaluate a null literal node
         */
        Value evaluateNull(NullNode* node);
    };

} // namespace expressions
} // namespace evaluator
