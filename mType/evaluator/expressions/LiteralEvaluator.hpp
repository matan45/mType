#pragma once

#include "../base/EvaluationContext.hpp"

namespace evaluator
{
    namespace expressions
    {
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
        class LiteralEvaluator
        {
        private:
            std::shared_ptr<EvaluationContext> context;

        public:
            explicit LiteralEvaluator(std::shared_ptr<EvaluationContext> ctx);

            Value evaluateInteger(IntegerNode* node);

            Value evaluateFloat(FloatNode* node);

            Value evaluateString(StringNode* node);

            Value evaluateBool(BoolNode* node);

            Value evaluateNull(NullNode* node);
        };
    }
}
