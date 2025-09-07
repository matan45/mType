#pragma once
#include "base/EvaluationContext.hpp"
#include "managers/ControlFlowManager.hpp"
#include "utils/ValueConverter.hpp"
#include "../ast/NodeClassesDeclaration.hpp"
#include "../ast/nodes/statements/BreakNode.hpp"
#include "../errors/SourceLocation.hpp"
#include <memory>

namespace evaluator
{
    using namespace base;
    using namespace managers;
    using namespace utils;
    using namespace value;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;

    /**
     * @brief Refactored Statement Evaluator following SOLID principles
     * - Single Responsibility: Only handles statement evaluation
     * - Open/Closed: Extensible through composition
     * - Liskov Substitution: Implements IStatementEvaluator interface
     * - Interface Segregation: Uses specialized interfaces
     * - Dependency Inversion: Depends on abstractions
     */
    class StatementEvaluator
    {
    private:
        std::shared_ptr<EvaluationContext> context;
        std::unique_ptr<ControlFlowManager> flowManager;

        // Forward declarations for circular dependency resolution
        class ExpressionEvaluator* exprEvaluator;
        class ObjectEvaluator* objEvaluator;

        // Helper method for import evaluation
        void evaluateRecursively(ASTNode* node);

    public:
        explicit StatementEvaluator(std::shared_ptr<EvaluationContext> ctx);
        ~StatementEvaluator() = default;

        // IEvaluator interface implementation
        Value evaluate(ASTNode* node);
        bool canHandle(ASTNode* node) const;

        // IStatementEvaluator interface implementation
        bool shouldBreakOrContinue() const;
        void handleBreak();
        void handleContinue();
        void resetLoopFlags();

        // Statement evaluation methods
        Value evaluateProgramNode(ProgramNode* node);
        Value evaluateBlockNode(BlockNode* node);
        Value evaluateDeclarationNode(DeclarationNode* node);
        Value evaluateAssignmentNode(AssignmentNode* node);
        Value evaluateIfNode(IfNode* node);
        Value evaluateWhileNode(WhileNode* node);
        Value evaluateDoWhileNode(DoWhileNode* node);
        Value evaluateForNode(ForNode* node);
        Value evaluateForEachNode(ForEachNode* node);
        Value evaluateBreakNode(BreakNode* node);
        Value evaluateContinueNode(ContinueNode* node);
        Value evaluateSwitchNode(SwitchNode* node);
        Value evaluateCaseNode(CaseNode* node);
        Value evaluateDefaultCaseNode(DefaultCaseNode* node);
        Value evaluateImportNode(ImportNode* node);
        Value evaluateFunctionNode(FunctionNode* node);
        Value evaluateReturnNode(ReturnNode* node);
        Value evaluateNativeFunctionNode(NativeFunctionNode* node);

        // Dependency injection for cross-evaluator communication
        void setExpressionEvaluator(ExpressionEvaluator* evaluator);
        void setObjectEvaluator(ObjectEvaluator* evaluator);

    private:
        // Helper methods
        bool isStatementNode(ASTNode* node) const;
        Value executeStatementList(const std::vector<std::unique_ptr<ASTNode>>& statements);
        void validateVariableDeclaration(DeclarationNode* node);
        void validateAssignmentAsDeclaration(AssignmentNode* node);
        void validateTypeAssignment(ValueType expectedType, const Value& value,
                                    const std::string& variableName,
                                    const SourceLocation& location);
        void validateTypeAssignment(ValueType expectedType, const Value& value,
                                    const std::string& variableName,
                                    const SourceLocation& location,
                                    const std::string& expectedClassName);
        void validateClassExists(const std::string& className, const SourceLocation& location);
        bool isValidTypeConversion(ValueType from, ValueType to);
        void validateObjectTypeCompatibility(const Value& value, const std::string& variableName,
                                             const SourceLocation& location);
        void validateObjectTypeCompatibility(const Value& value, const std::string& variableName,
                                             const SourceLocation& location,
                                             const std::string& expectedClassName);
    };
}
