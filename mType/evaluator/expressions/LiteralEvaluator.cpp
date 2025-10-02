#include "LiteralEvaluator.hpp"
#include "../../value/StringPool.hpp"
#include "../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../ast/nodes/expressions/StringNode.hpp"
#include "../../ast/nodes/expressions/FloatNode.hpp"
#include "../../ast/nodes/expressions/BoolNode.hpp"

namespace evaluator
{
    namespace expressions
    {
        LiteralEvaluator::LiteralEvaluator(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx)
        {
        }

        Value LiteralEvaluator::evaluateInteger(IntegerNode* node)
        {
            return node->getValue();
        }

        Value LiteralEvaluator::evaluateFloat(FloatNode* node)
        {
            return node->getValue();
        }

        Value LiteralEvaluator::evaluateString(StringNode* node)
        {
            // Use string pool for memory efficiency
            auto& pool = value::StringPool::getInstance();
            return pool.intern(node->getValue());
        }

        Value LiteralEvaluator::evaluateBool(BoolNode* node)
        {
            return node->getValue();
        }

        Value LiteralEvaluator::evaluateNull(NullNode* node)
        {
            return nullptr;
        }
    } 
}
