#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../../value/ValueType.hpp"
#include <memory>

// Forward declarations
namespace ast
{
    namespace nodes
    {
        namespace expressions
        {
            class VariableNode;
        }

        namespace classes
        {
            class MemberAccessNode;
            class NewNode;
        }

        namespace statements
        {
            class AssignmentNode;
        }
    }
}

namespace evaluator
{
    class ExpressionEvaluator;
    class ObjectEvaluator;
    class StatementEvaluator;
}

namespace evaluator
{
    namespace expressions
    {
        using namespace base;
        using namespace value;
        using namespace ast::nodes::expressions;
        using namespace ast::nodes::classes;
        using namespace ast::nodes::statements;

        /**
         * @brief Handles access operations (variables, members, objects, assignment)
         *
         * Responsibilities:
         * - Variable access (local, instance, static, qualified)
         * - Member access (object fields, array length)
         * - Object creation (new expressions)
         * - Assignment expressions
         * - 'this' keyword handling
         *
         * Design Principles:
         * - Single Responsibility: Only access operations
         * - Handles complex variable resolution logic
         * - Supports qualified static access (ClassName::fieldName)
         */
        class AccessHandler
        {
        private:
            std::shared_ptr<EvaluationContext> context;
            ExpressionEvaluator* exprEvaluator;
            ObjectEvaluator* objEvaluator;
            StatementEvaluator* stmtEvaluator;

            // Helper methods for variable access
            Value handleThisKeyword(VariableNode* node);
            Value handleQualifiedStaticAccess(const std::string& varName, VariableNode* node);
            Value handleInstanceFieldAccess(const std::string& varName, VariableNode* node);
            Value handleStaticFieldAccess(const std::string& varName, VariableNode* node);
            std::vector<std::string> parseQualifiedName(const std::string& qualifiedName);

        public:
            explicit AccessHandler(std::shared_ptr<EvaluationContext> ctx);

            void setExpressionEvaluator(ExpressionEvaluator* evaluator);

            void setObjectEvaluator(ObjectEvaluator* evaluator);

            void setStatementEvaluator(StatementEvaluator* evaluator);


            Value evaluateVariableAccess(VariableNode* node);

            Value evaluateMemberAccess(MemberAccessNode* node);

            Value evaluateObjectCreation(NewNode* node);

            Value evaluateAssignment(AssignmentNode* node);
        };
    } 
} 
