#pragma once

#include "../base/EvaluationContext.hpp"
#include "../interfaces/IExpressionEvaluator.hpp"
#include "../interfaces/IStatementEvaluator.hpp"
#include "../interfaces/IObjectEvaluator.hpp"
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
     * - Dependency Inversion: Depends on interfaces
     * - Manages complex generic type resolution
     */
    class GenericInstantiationHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        interfaces::IExpressionEvaluator* exprEvaluator;
        interfaces::IStatementEvaluator* stmtEvaluator;
        interfaces::IObjectEvaluator* objEvaluator;

    public:
        explicit GenericInstantiationHandler(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr), stmtEvaluator(nullptr), objEvaluator(nullptr) {}

        void setExpressionEvaluator(interfaces::IExpressionEvaluator* evaluator) {
            exprEvaluator = evaluator;
        }

        void setStatementEvaluator(interfaces::IStatementEvaluator* evaluator) {
            stmtEvaluator = evaluator;
        }

        void setObjectEvaluator(interfaces::IObjectEvaluator* evaluator) {
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

        // Helper methods for evaluateNew refactoring
        std::string resolveClassName(const std::string& className);
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> findOrInstantiateClass(
            const std::string& className, const ast::nodes::classes::NewNode* node);
        std::unordered_map<std::string, std::string> extractGenericTypeBindings(
            const std::string& classNameForInstance);
    };

} // namespace objects
} // namespace evaluator
