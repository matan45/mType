#pragma once

#include "../base/EvaluationContext.hpp"
#include "../interfaces/IExpressionEvaluator.hpp"
#include "../interfaces/IStatementEvaluator.hpp"
#include "../interfaces/IObjectEvaluator.hpp"
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
         * - Dependency Inversion: Depends on interfaces, not concrete classes
         */
        class CallHandler
        {
        private:
            std::shared_ptr<EvaluationContext> context;
            interfaces::IExpressionEvaluator* exprEvaluator;
            interfaces::IStatementEvaluator* stmtEvaluator;
            interfaces::IObjectEvaluator* objEvaluator;

        public:
            explicit CallHandler(std::shared_ptr<EvaluationContext> ctx);


            void setExpressionEvaluator(interfaces::IExpressionEvaluator* evaluator);

            void setStatementEvaluator(interfaces::IStatementEvaluator* evaluator);

            void setObjectEvaluator(interfaces::IObjectEvaluator* evaluator);
            
            
            Value evaluateFunctionCall(FunctionCallNode* node);

            Value evaluateMethodCall(MethodCallNode* node);

            Value evaluateLambdaInterfaceInvocation(LambdaInterfaceInvocationNode* node);

        private:
            // Helper methods for evaluateFunctionCall - breaking down 395-line function
            std::optional<Value> tryNativeFunctionCall(ast::nodes::functions::FunctionCallNode* node);
            std::optional<Value> tryQualifiedMethodCall(ast::nodes::functions::FunctionCallNode* node);
            std::optional<Value> tryInstanceMethodCall(ast::nodes::functions::FunctionCallNode* node);
            std::optional<Value> tryStaticMethodCall(ast::nodes::functions::FunctionCallNode* node);
            std::vector<Value> evaluateArguments(const std::vector<std::unique_ptr<ASTNode>>& args);
            std::string resolveClassNameFromThis(const std::string& className, const errors::SourceLocation& location);
        };
    }
}
