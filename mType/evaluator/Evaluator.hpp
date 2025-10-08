#pragma once
#include "../ast/ASTVisitor.hpp"
#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"
#include "../ast/NodeClassesDeclaration.hpp"
#include "EvaluatorCoordinator.hpp"
#include <memory>

namespace evaluator
{
    using namespace ast;
    using namespace value;
    using namespace environment;

    /**
     * @brief Compatibility wrapper around the new EvaluatorCoordinator
     * Maintains the original API for existing code while using the refactored architecture internally
     */
    class Evaluator : public ASTVisitor<Value>
    {
    private:
        std::unique_ptr<EvaluatorCoordinator> coordinator;
        
    public:
        explicit Evaluator(std::shared_ptr<Environment> environment);
        ~Evaluator() override;

        Value evaluate(ASTNode* node);
        
        // Visitor methods for all node types
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
        Value visitIndexAssignmentNode(IndexAssignmentNode* node) override;
        Value visitMethodCallNode(MethodCallNode* node) override;
        Value visitMemberAccessNode(MemberAccessNode* node) override;
        Value visitNewNode(NewNode* node) override;
        Value visitMethodNode(MethodNode* node) override;
        Value visitConstructorNode(ConstructorNode* node) override;
        Value visitFieldNode(FieldNode* node) override;
        Value visitClassNode(ClassNode* node) override;
        Value visitInterfaceNode(InterfaceNode* node) override;
        Value visitArrayCreationNode(ArrayCreationNode* node) override;
        Value visitArrayLiteralNode(ArrayLiteralNode* node) override;
        Value visitIndexAccessNode(IndexAccessNode* node) override;
        Value visitForEachNode(ForEachNode* node) override;
        Value visitLambdaNode(LambdaNode* node) override;
        Value visitSuperConstructorCallNode(SuperConstructorCallNode* node) override;
        Value visitSuperMethodCallNode(SuperMethodCallNode* node) override;
        Value visitCastExpression(CastExpression* node) override;
        Value visitInstanceOfExpression(InstanceOfExpression* node) override;
        Value visitAwaitExpression(AwaitExpression* node) override;

        // Compatibility methods - delegate to coordinator
        std::shared_ptr<Environment> getEnvironment() const;
        std::shared_ptr<base::EvaluationContext> getContext() const;
        bool shouldReturn() const;
        void setReturned(bool returned);
        Value getReturnValue();
        void pushReturnValue(const Value& value);

        // Object instance access
        std::shared_ptr<ObjectInstance> getCurrentInstance() const;
        
        // Type conversion helpers
        bool isTruthy(const Value& value);
        std::string toString(const Value& value);
        float toFloat(const Value& value);
        int toInt(const Value& value);
        
        // Cross-evaluator helpers
        Value evaluateObjectMethodCall(MethodCallNode* node);
        Value evaluateObjectCreation(NewNode* node);
        Value evaluateObjectMemberAccess(MemberAccessNode* node);
        Value evaluateObjectMemberAssignment(MemberAssignmentNode* node);
        Value callMethodOnInstance(std::shared_ptr<ObjectInstance> instance, 
                                   const std::string& methodName, const std::vector<Value>& args);
        
    };
}
