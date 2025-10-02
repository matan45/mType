#include "ArgumentEvaluator.hpp"
#include "../ExpressionEvaluator.hpp"

namespace evaluator {
namespace utils {

    std::vector<Value> ArgumentEvaluator::evaluateArguments(
        const std::vector<std::unique_ptr<ast::ASTNode>>& args,
        ExpressionEvaluator* evaluator)
    {
        std::vector<Value> values;
        values.reserve(args.size());

        if (evaluator)
        {
            for (const auto& arg : args)
            {
                values.push_back(evaluator->evaluate(arg.get()));
            }
        }

        return values;
    }

} // namespace utils
} // namespace evaluator
