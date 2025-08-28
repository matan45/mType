#pragma once
#include "../ast/ASTVisitor.hpp"
#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"
#include "../ast/nodes/statements/DeclarationNode.hpp"
#include "../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../ast/nodes/statements/NativeFunctionNode.hpp"
#include <memory>
#include <stack>

namespace evaluator
{
    using namespace ast;
    using namespace value;
    using namespace environment;

    class ExpressionEvaluator;
    class StatementEvaluator;
    class ObjectEvaluator;
    class NamespaceEvaluator;

    class Evaluator : public ASTVisitor<Value>
    {
    private:
        std::shared_ptr<environment::Environment> env;
        std::unique_ptr<ExpressionEvaluator> exprEvaluator;
        std::unique_ptr<StatementEvaluator> stmtEvaluator;
        std::unique_ptr<ObjectEvaluator> objEvaluator;
        std::unique_ptr<NamespaceEvaluator> nsEvaluator;
        
        std::stack<Value> returnStack;
        bool hasReturned;
        
    public:
        explicit Evaluator(std::shared_ptr<environment::Environment> environment);
        ~Evaluator();

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
        Value visitMethodCallNode(MethodCallNode* node) override;
        Value visitMemberAccessNode(MemberAccessNode* node) override;
        Value visitNewNode(NewNode* node) override;
        Value visitMethodNode(MethodNode* node) override;
        Value visitConstructorNode(ConstructorNode* node) override;
        Value visitFieldNode(FieldNode* node) override;
        Value visitClassNode(ClassNode* node) override;
        Value visitNamespaceNode(NamespaceNode* node) override;
        Value visitUsingNode(UsingNode* node) override;
        Value visitQualifiedNameNode(QualifiedNameNode* node) override;
        Value visitQualifiedAssignmentNode(QualifiedAssignmentNode* node) override;
        
        // Helper methods
        std::shared_ptr<environment::Environment> getEnvironment() const { return env; }
        bool shouldReturn() const { return hasReturned; }
        void setReturned(bool returned) { hasReturned = returned; }
        Value getReturnValue();
        void pushReturnValue(const Value& value);
        
        // Helper for truthiness checking (delegates to ExpressionEvaluator)
        bool isTruthy(const Value& value);
        
        // Helper for type conversion (delegates to ExpressionEvaluator)  
        std::string toString(const Value& value);
        float toFloat(const Value& value);
        int toInt(const Value& value);
        
        // Helpers for cross-evaluator delegation
        Value evaluateObjectMethodCall(MethodCallNode* node);
        Value evaluateObjectCreation(NewNode* node);
        Value evaluateObjectMemberAccess(MemberAccessNode* node);
        Value evaluateObjectMemberAssignment(MemberAssignmentNode* node);
        Value evaluateQualifiedNameAccess(QualifiedNameNode* node);
        Value evaluateQualifiedAssignment(QualifiedAssignmentNode* node);
    };
}
