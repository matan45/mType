#include "LiteralEvaluator.hpp"
#include "../../value/StringPool.hpp"

namespace evaluator {
namespace expressions {

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

} // namespace expressions
} // namespace evaluator
