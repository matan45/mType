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
}

namespace evaluator
{
    namespace statements
    {
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
        class DeclarationHandler
        {
        private:
            std::shared_ptr<EvaluationContext> context;
            evaluator::ExpressionEvaluator* exprEvaluator;
            evaluator::ObjectEvaluator* objEvaluator;

        public:
            explicit DeclarationHandler(std::shared_ptr<EvaluationContext> ctx)
                : context(ctx), exprEvaluator(nullptr), objEvaluator(nullptr)
            {
            }

            void setExpressionEvaluator(evaluator::ExpressionEvaluator* evaluator)
            {
                exprEvaluator = evaluator;
            }

            void setObjectEvaluator(evaluator::ObjectEvaluator* evaluator)
            {
                objEvaluator = evaluator;
            }

            /**
             * Evaluate variable assignment (includes complex logic for declarations, static fields, etc.)
             */
            Value evaluateAssignment(AssignmentNode* node);

        private:
            // Assignment type classification
            enum class AssignmentType
            {
                LOCAL_VARIABLE_ASSIGNMENT,    // x = 5 (x is local/param)
                LOCAL_VARIABLE_SHADOWING,     // int x = 5 (shadows outer x)
                INSTANCE_FIELD_ASSIGNMENT,    // field = 5 (in instance method)
                GLOBAL_VARIABLE_ASSIGNMENT,   // x = 5 (x is global)
                GLOBAL_VARIABLE_SHADOWING,    // int x = 5 (shadows global)
                NEW_VARIABLE_DECLARATION,     // int x = 5 (first declaration)
                QUALIFIED_STATIC_FIELD,       // Class::field = 5
                UNQUALIFIED_STATIC_FIELD      // field = 5 (in static context)
            };

            // Decision helpers
            bool isDeclaration(AssignmentNode* node) const;
            bool isLocalOrParameter(std::shared_ptr<runtimeTypes::global::VariableDefinition> varDef) const;
            bool isQualifiedStatic(const std::string& varName) const;
            AssignmentType determineAssignmentType(AssignmentNode* node,
                                                  std::shared_ptr<runtimeTypes::global::VariableDefinition> varDef);

            // Validation helpers
            void validateAssignmentAsDeclaration(AssignmentNode* node);
            void validateClassExists(const std::string& className, const SourceLocation& location);
            void validateTypeAssignment(ValueType expectedType, const Value& value,
                                        const std::string& variableName,
                                        const SourceLocation& location);
            void validateTypeAssignment(ValueType expectedType, const Value& value,
                                        const std::string& variableName,
                                        const SourceLocation& location,
                                        const std::string& expectedClassName);

            // Assignment handling helpers
            Value handleLambdaConversion(Value value, AssignmentNode* node);
            bool tryImplicitFieldAssignment(const Value& newValue, AssignmentNode* node);
            Value handleNewVariableDeclaration(const Value& newValue, AssignmentNode* node);
            Value handleQualifiedStaticAssignment(const Value& newValue, AssignmentNode* node);
            Value handleUnqualifiedStaticAssignment(const Value& newValue, AssignmentNode* node);
            Value handleScopeShadowing(const Value& newValue, AssignmentNode* node,
                                       std::shared_ptr<runtimeTypes::global::VariableDefinition> varDef);
            Value handleExistingVariableAssignment(const Value& newValue, AssignmentNode* node,
                                                   std::shared_ptr<runtimeTypes::global::VariableDefinition> varDef);

            // Utility helpers
            Value convertLambdaToInterface(const Value& lambdaValue, const std::string& interfaceName,
                                           const SourceLocation& location);
            std::shared_ptr<runtimeTypes::global::VariableDefinition> createVariableDefinition(
                AssignmentNode* node, const Value& value);
        };
    } // namespace statements
} // namespace evaluator
