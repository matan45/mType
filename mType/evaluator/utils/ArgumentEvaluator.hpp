#pragma once

#include "../../value/ValueType.hpp"
#include "../../ast/ASTNode.hpp"
#include <vector>
#include <memory>

namespace evaluator {

    // Forward declaration to avoid circular dependency
    class ExpressionEvaluator;

namespace utils {

    using namespace value;

    /**
     * @brief Argument list evaluation utility
     *
     * This class provides consistent argument evaluation logic
     * for function calls, method calls, and constructors.
     *
     * Responsibilities:
     * - Evaluate argument lists for calls
     * - Consistent error handling for argument evaluation
     *
     * Design Principles:
     * - Single Responsibility: Only argument evaluation
     * - DRY: Eliminates duplicate argument evaluation code
     * - Stateless: All methods are static
     */
    class ArgumentEvaluator {
    public:
        /**
         * Evaluate a list of argument expressions
         * @param args The argument AST nodes to evaluate
         * @param evaluator The expression evaluator to use
         * @return Vector of evaluated argument values
         */
        static std::vector<Value> evaluateArguments(
            const std::vector<std::unique_ptr<ast::ASTNode>>& args,
            ExpressionEvaluator* evaluator);
    };

} // namespace utils
} // namespace evaluator
