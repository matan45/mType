#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../../value/ValueType.hpp"
#include <memory>

// Forward declarations
namespace ast {
namespace nodes {
namespace statements {
    class DeclarationNode;
    class AssignmentNode;
}
}
}

namespace evaluator {
    class ExpressionEvaluator;
    class ObjectEvaluator;
}

namespace evaluator {
namespace statements {

    using namespace base;
    using namespace value;
    using namespace ast::nodes::statements;

    /**
     * @brief Handles variable declarations and assignments
     *
     * Responsibilities:
     * - Variable declarations (with/without initialization)
     * - Variable assignments (simple, qualified static, field)
     * - Type validation and compatibility checking
     * - Final variable enforcement
     * - Lambda-to-interface conversion
     * - Generic type parameter resolution
     *
     * Design Principles:
     * - Single Responsibility: Only declaration and assignment logic
     * - Delegates type validation to TypeValidator utility
     */
    class DeclarationHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        evaluator::ExpressionEvaluator* exprEvaluator;
        evaluator::ObjectEvaluator* objEvaluator;

    public:
        explicit DeclarationHandler(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr), objEvaluator(nullptr) {}

        void setExpressionEvaluator(evaluator::ExpressionEvaluator* evaluator) {
            exprEvaluator = evaluator;
        }

        void setObjectEvaluator(evaluator::ObjectEvaluator* evaluator) {
            objEvaluator = evaluator;
        }

        /**
         * Evaluate variable declaration
         */
        Value evaluateDeclaration(DeclarationNode* node);

        /**
         * Evaluate variable assignment (includes complex logic for declarations, static fields, etc.)
         */
        Value evaluateAssignment(AssignmentNode* node);

    private:
        // Helper methods
        void validateVariableDeclaration(DeclarationNode* node);
        void validateAssignmentAsDeclaration(AssignmentNode* node);
        void validateClassExists(const std::string& className, const SourceLocation& location);
        void validateTypeAssignment(ValueType expectedType, const Value& value,
                                   const std::string& variableName,
                                   const SourceLocation& location);
        void validateTypeAssignment(ValueType expectedType, const Value& value,
                                   const std::string& variableName,
                                   const SourceLocation& location,
                                   const std::string& expectedClassName);
        Value convertLambdaToInterface(const Value& lambdaValue, const std::string& interfaceName,
                                      const SourceLocation& location);
    };

} // namespace statements
} // namespace evaluator
