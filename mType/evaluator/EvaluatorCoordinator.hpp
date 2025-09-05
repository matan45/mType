#pragma once
#include "../ast/ASTVisitor.hpp"
#include "base/EvaluationContext.hpp"
#include "ExpressionEvaluator.hpp"
#include "StatementEvaluator.hpp"
#include "ObjectEvaluator.hpp"
#include "../value/ValueType.hpp"
#include <memory>

namespace evaluator
{
    using namespace base;
    using namespace ast;
    using namespace value;
    
    /**
     * @brief Coordinator class that orchestrates evaluation across specialized evaluators
     * Following the Coordinator Pattern and SOLID principles:
     * - Single Responsibility: Coordinates evaluation, doesn't do evaluation itself
     * - Open/Closed: New evaluators can be added without modifying existing code
     * - Liskov Substitution: Can substitute any IEvaluator implementations
     * - Interface Segregation: Uses specific interfaces for each evaluator type
     * - Dependency Inversion: Depends on abstractions, not concrete classes
     */
    class EvaluatorCoordinator : public ASTVisitor<Value>
    {
    private:
        std::shared_ptr<EvaluationContext> context;
        std::unique_ptr<ExpressionEvaluator> exprEvaluator;
        std::unique_ptr<StatementEvaluator> stmtEvaluator;
        std::unique_ptr<ObjectEvaluator> objEvaluator;
        
    public:
        explicit EvaluatorCoordinator(std::shared_ptr<Environment> environment);
        ~EvaluatorCoordinator() override = default;
        
        // Main evaluation entry point
        Value evaluate(ASTNode* node);
        
        // ASTVisitor interface implementation (delegates to appropriate evaluator)
        Value visitProgramNode(ProgramNode* node) override;
        Value visitBlockNode(BlockNode* node) override;
        Value visitFloatNode(FloatNode* node) override;
        Value visitIntegerNode(IntegerNode* node) override;
        Value visitStringNode(StringNode* node) override;
        Value visitBoolNode(BoolNode* node) override;
        Value visitVariableNode(VariableNode* node) override;
        Value visitDeclarationNode(DeclarationNode* node) override;
        Value visitAssignmentNode(AssignmentNode* node) override;
        Value visitBinaryOpNode(BinaryOpNode* node) override;
        Value visitTernaryOpNode(TernaryOpNode* node) override;
        Value visitUnaryOpNode(UnaryOpNode* node) override;
        Value visitIfNode(IfNode* node) override;
        Value visitWhileNode(WhileNode* node) override;
        Value visitDoWhileNode(DoWhileNode* node) override;
        Value visitForNode(ForNode* node) override;
        Value visitBreakNode(BreakNode* node) override;
        Value visitContinueNode(ContinueNode* node) override;
        Value visitFunctionNode(FunctionNode* node) override;
        Value visitFunctionCallNode(FunctionCallNode* node) override;
        Value visitReturnNode(ReturnNode* node) override;
        Value visitSwitchNode(SwitchNode* node) override;
        Value visitCaseNode(CaseNode* node) override;
        Value visitImportNode(ImportNode* node) override;
        Value visitDefaultCaseNode(DefaultCaseNode* node) override;
        Value visitNativeFunctionNode(NativeFunctionNode* node) override;
        Value visitNullNode(NullNode* node) override;
        Value visitMemberAssignmentNode(MemberAssignmentNode* node) override;
        Value visitMethodCallNode(MethodCallNode* node) override;
        Value visitMemberAccessNode(MemberAccessNode* node) override;
        Value visitNewNode(NewNode* node) override;
        Value visitMethodNode(MethodNode* node) override;
        Value visitConstructorNode(ConstructorNode* node) override;
        Value visitFieldNode(FieldNode* node) override;
        Value visitClassNode(ClassNode* node) override;
        
        // Context and evaluator access methods
        std::shared_ptr<EvaluationContext> getContext() const { return context; }
        ExpressionEvaluator* getExpressionEvaluator() const { return exprEvaluator.get(); }
        StatementEvaluator* getStatementEvaluator() const { return stmtEvaluator.get(); }
        ObjectEvaluator* getObjectEvaluator() const { return objEvaluator.get(); }
        
        // Helper methods for cross-evaluator operations (maintained for compatibility)
        bool isTruthy(const Value& value) const;
        std::string toString(const Value& value) const;
        float toFloat(const Value& value) const;
        int toInt(const Value& value) const;
        
        // State management (maintained for compatibility)
        bool shouldReturn() const;
        void setReturned(bool returned);
        Value getReturnValue();
        void pushReturnValue(const Value& value);
        std::shared_ptr<ObjectInstance> getCurrentInstance() const;
        
    private:
        // Setup cross-evaluator dependencies
        void setupEvaluatorDependencies();
        
        // Route evaluation to appropriate evaluator
        Value routeEvaluation(ASTNode* node);
    };
}