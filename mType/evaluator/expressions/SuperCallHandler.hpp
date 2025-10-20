#pragma once

#include "../base/EvaluationContext.hpp"
#include "../interfaces/IExpressionEvaluator.hpp"
#include "../interfaces/IStatementEvaluator.hpp"
#include "../interfaces/IObjectEvaluator.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../../value/ValueType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <memory>

namespace evaluator
{
    namespace expressions
    {
        using namespace base;
        using namespace value;
        using namespace ast::nodes::classes;

        /**
         * @brief Handles super constructor and method calls for inheritance
         *
         * Responsibilities:
         * - Super constructor calls: super(args)
         * - Super method calls: super.methodName(args)
         * - Circular inheritance detection
         * - Calling class stack management
         * - Parameter binding with generic type support
         *
         * Design Principles:
         * - Single Responsibility: Only handles super-related operations
         * - Dependency Inversion: Depends on interfaces, not concrete classes
         * - Manages complex inheritance resolution logic
         */
        class SuperCallHandler
        {
        private:
            std::shared_ptr<EvaluationContext> context;
            interfaces::IExpressionEvaluator* exprEvaluator;
            interfaces::IStatementEvaluator* stmtEvaluator;
            interfaces::IObjectEvaluator* objEvaluator;

        public:
            explicit SuperCallHandler(std::shared_ptr<EvaluationContext> ctx);

            void setExpressionEvaluator(interfaces::IExpressionEvaluator* evaluator);
            void setStatementEvaluator(interfaces::IStatementEvaluator* evaluator);
            void setObjectEvaluator(interfaces::IObjectEvaluator* evaluator);

            Value evaluateSuperConstructorCall(SuperConstructorCallNode* node);
            Value evaluateSuperMethodCall(SuperMethodCallNode* node);
        };
    }
}
