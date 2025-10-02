#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../value/ValueType.hpp"
#include "../../ast/nodes/classes/NewNode.hpp"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

// Forward declarations
namespace runtimeTypes {
namespace klass {
    class ObjectInstance;
}
}

namespace evaluator {
    class ExpressionEvaluator;
    class StatementEvaluator;
    class ObjectEvaluator;
}

namespace evaluator {
namespace objects {

    using namespace base;
    using namespace value;
    using namespace ast::nodes::classes;

    /**
     * @brief Handles generic class instantiation and object creation
     *
     * Responsibilities:
     * - Resolving generic type parameters
     * - Instantiating generic classes
     * - Creating object instances with type bindings
     * - Executing constructors
     *
     * Design Principles:
     * - Single Responsibility: Only handles new/instantiation logic
     * - Delegates instance creation to InstanceOperationHandler
     * - Manages complex generic type resolution
     */
    class GenericInstantiationHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        evaluator::ExpressionEvaluator* exprEvaluator;
        evaluator::StatementEvaluator* stmtEvaluator;
        evaluator::ObjectEvaluator* objEvaluator;

    public:
        explicit GenericInstantiationHandler(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr), stmtEvaluator(nullptr), objEvaluator(nullptr) {}

        void setExpressionEvaluator(evaluator::ExpressionEvaluator* evaluator) {
            exprEvaluator = evaluator;
        }

        void setStatementEvaluator(evaluator::StatementEvaluator* evaluator) {
            stmtEvaluator = evaluator;
        }

        void setObjectEvaluator(evaluator::ObjectEvaluator* evaluator) {
            objEvaluator = evaluator;
        }

        /**
         * Evaluate a new object creation node
         */
        Value evaluateNew(NewNode* node);

    private:
        /**
         * Evaluate argument list for constructor
         */
        std::vector<Value> evaluateArgumentList(const std::vector<std::unique_ptr<ast::ASTNode>>& args);

        /**
         * Resolve type parameter from current context
         */
        std::string resolveTypeParameterFromContext(const std::string& typeParam);
    };

} // namespace objects
} // namespace evaluator
