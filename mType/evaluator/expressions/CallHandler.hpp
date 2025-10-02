#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../../value/ValueType.hpp"
#include <memory>

namespace ast
{
    namespace nodes
    {
        namespace expressions
        {
            class LambdaInterfaceInvocationNode;
        }

        namespace functions
        {
            class FunctionCallNode;
        }

        namespace classes
        {
            class MethodCallNode;
        }
    }
}

namespace evaluator
{
    class ExpressionEvaluator;
    class StatementEvaluator;
    class ObjectEvaluator;
}

namespace evaluator
{
    namespace expressions
    {
        using namespace base;
        using namespace value;
        using namespace ast::nodes::expressions;
        using namespace ast::nodes::classes;

        /**
         * @brief Handles function and method call evaluation
         *
         * Responsibilities:
         * - Function call evaluation (global and namespaced)
         * - Method call evaluation (instance and static)
         * - Lambda invocation
         * - Argument evaluation and passing
         *
         * Design Principles:
         * - Single Responsibility: Only call operations
         * - Handles generic type resolution for calls
         */
        class CallHandler
        {
        private:
            std::shared_ptr<EvaluationContext> context;
            ExpressionEvaluator* exprEvaluator;
            StatementEvaluator* stmtEvaluator;
            ObjectEvaluator* objEvaluator;

        public:
            explicit CallHandler(std::shared_ptr<EvaluationContext> ctx);
          

            void setExpressionEvaluator(ExpressionEvaluator* evaluator);

            void setStatementEvaluator(StatementEvaluator* evaluator);
            
            void setObjectEvaluator(ObjectEvaluator* evaluator);
            
            
            Value evaluateFunctionCall(FunctionCallNode* node);
            
            Value evaluateMethodCall(MethodCallNode* node);
            
            Value evaluateLambdaInterfaceInvocation(LambdaInterfaceInvocationNode* node);
        };
    } 
} 
