#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../value/ValueType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <memory>
#include <string>

// Forward declarations
namespace ast {
namespace nodes {
namespace statements {
    class ImportNode;
    class NativeFunctionNode;
}
namespace functions {
    class FunctionNode;
}
}
}

namespace evaluator {
    class ExpressionEvaluator;
    class StatementEvaluator;
}

namespace evaluator {
namespace statements {

    using namespace base;
    using namespace value;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;

    /**
     * @brief Handles import and function registration
     *
     * Responsibilities:
     * - Import statement processing
     * - Function definition registration
     * - Native function registration
     * - Lambda-to-interface conversion
     *
     * Design Principles:
     * - Single Responsibility: Only imports and function registration
     * - Delegates to ImportManager for actual import logic
     * - Handles function/native function registration
     */
    class ImportAndFunctionHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        evaluator::ExpressionEvaluator* exprEvaluator;
        evaluator::StatementEvaluator* stmtEvaluator;

    public:
        explicit ImportAndFunctionHandler(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr), stmtEvaluator(nullptr) {}

        void setExpressionEvaluator(evaluator::ExpressionEvaluator* evaluator) {
            exprEvaluator = evaluator;
        }

        void setStatementEvaluator(evaluator::StatementEvaluator* evaluator) {
            stmtEvaluator = evaluator;
        }

        /**
         * Evaluate an import statement
         */
        Value evaluateImport(ImportNode* node);

        /**
         * Evaluate a function definition
         */
        Value evaluateFunction(FunctionNode* node);

        /**
         * Evaluate a native function definition
         */
        Value evaluateNativeFunction(ast::nodes::statements::NativeFunctionNode* node);

        /**
         * Convert a lambda value to an interface implementation
         */
        Value convertLambdaToInterface(const Value& lambdaValue, const std::string& interfaceName,
                                      const errors::SourceLocation& location = errors::SourceLocation{});
    };

} // namespace statements
} // namespace evaluator
