#pragma once

namespace ast
{
    // Forward declarations of all AST node types
    class ProgramNode;
    class BlockNode;
    class NumberNode;
    class StringNode;
    class BoolNode;
    class VariableNode;
    class DeclarationNode;
    class AssignmentNode;
    class BinaryOpNode;
    class TernaryOpNode;
    class UnaryOpNode;
    class IfNode;
    class WhileNode;
    class DoWhileNode;
    class ForNode;
    class BreakNode;
    class ContinueNode;
    class FunctionNode;
    class FunctionCallNode;
    class ReturnNode;
    class SwitchNode;
    class CaseNode;
    class DefaultCaseNode;
    class NativeFunctionNode;
    class ImportNode;
    class ClassNode;
    class FieldNode;
    class ConstructorNode;
    class MethodNode;
    class NewNode;
    class MemberAccessNode;
    class MethodCallNode;
    class MemberAssignmentNode;
    class NullNode;
    class NamespaceNode;
    class UsingNode;
    class QualifiedNameNode;
    class QualifiedAssignmentNode;

    // Abstract visitor interface
    template<typename T>
    class ASTVisitor
    {
    public:
        virtual ~ASTVisitor() = default;

        // Visit methods for each node type
        virtual T visitProgramNode(ProgramNode* node) = 0;
        virtual T visitBlockNode(BlockNode* node) = 0;
        virtual T visitNumberNode(NumberNode* node) = 0;
        virtual T visitStringNode(StringNode* node) = 0;
        virtual T visitBoolNode(BoolNode* node) = 0;
        virtual T visitVariableNode(VariableNode* node) = 0;
        virtual T visitDeclarationNode(DeclarationNode* node) = 0;
        virtual T visitAssignmentNode(AssignmentNode* node) = 0;
        virtual T visitBinaryOpNode(BinaryOpNode* node) = 0;
        virtual T visitTernaryOpNode(TernaryOpNode* node) = 0;
        virtual T visitUnaryOpNode(UnaryOpNode* node) = 0;
        virtual T visitIfNode(IfNode* node) = 0;
        virtual T visitWhileNode(WhileNode* node) = 0;
        virtual T visitDoWhileNode(DoWhileNode* node) = 0;
        virtual T visitForNode(ForNode* node) = 0;
        virtual T visitBreakNode(BreakNode* node) = 0;
        virtual T visitContinueNode(ContinueNode* node) = 0;
        virtual T visitFunctionNode(FunctionNode* node) = 0;
        virtual T visitFunctionCallNode(FunctionCallNode* node) = 0;
        virtual T visitReturnNode(ReturnNode* node) = 0;
        virtual T visitSwitchNode(SwitchNode* node) = 0;
        virtual T visitCaseNode(CaseNode* node) = 0;
        virtual T visitImportNode(ImportNode* node) = 0;
        virtual T visitDefaultCaseNode(DefaultCaseNode* node) = 0;
        virtual T visitNativeFunctionNode(NativeFunctionNode* node) = 0;
        virtual T visitNullNode(NullNode* node) = 0;
        virtual T visitMemberAssignmentNode(MemberAssignmentNode* node) = 0;
        virtual T visitMethodCallNode(MethodCallNode* node) = 0;
        virtual T visitMemberAccessNode(MemberAccessNode* node) = 0;
        virtual T visitNewNode(NewNode* node) = 0;
        virtual T visitMethodNode(MethodNode* node) = 0;
        virtual T visitConstructorNode(ConstructorNode* node) = 0;
        virtual T visitFieldNode(FieldNode* node) = 0;
        virtual T visitClassNode(ClassNode* node) = 0;
        virtual T visitNamespaceNode(NamespaceNode* node) = 0;
        virtual T visitUsingNode(UsingNode* node) = 0;
        virtual T visitQualifiedNameNode(QualifiedNameNode* node) = 0;
        virtual T visitQualifiedAssignmentNode(QualifiedAssignmentNode* node) = 0;
    };
}