#pragma once

#include "../base/EvaluationContext.hpp"
#include "../interfaces/IExpressionEvaluator.hpp"
#include "../interfaces/IObjectEvaluator.hpp"
#include "../interfaces/IStatementEvaluator.hpp"
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
         * - Dependency Inversion: Depends on interfaces
         */
        class AccessHandler
        {
        private:
            std::shared_ptr<EvaluationContext> context;
            interfaces::IExpressionEvaluator* exprEvaluator;
            interfaces::IObjectEvaluator* objEvaluator;
            interfaces::IStatementEvaluator* stmtEvaluator;

            // Helper methods for variable access
            Value handleThisKeyword(VariableNode* node);
            Value handleQualifiedStaticAccess(const std::string& varName, VariableNode* node);
            Value handleInstanceFieldAccess(const std::string& varName, VariableNode* node);
            Value handleStaticFieldAccess(const std::string& varName, VariableNode* node);
            std::vector<std::string> parseQualifiedName(const std::string& qualifiedName);

        public:
            explicit AccessHandler(std::shared_ptr<EvaluationContext> ctx);

            void setExpressionEvaluator(interfaces::IExpressionEvaluator* evaluator);

            void setObjectEvaluator(interfaces::IObjectEvaluator* evaluator);

            void setStatementEvaluator(interfaces::IStatementEvaluator* evaluator);


            Value evaluateVariableAccess(VariableNode* node);

            Value evaluateMemberAccess(MemberAccessNode* node);

            Value evaluateObjectCreation(NewNode* node);

            Value evaluateAssignment(AssignmentNode* node);
        };
    } 
} 
