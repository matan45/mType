#pragma once
#include "NodeClassesDeclaration.hpp"

namespace ast
{
    // Abstract visitor interface
    template <typename T>
    class ASTVisitor
    {
    public:
        virtual ~ASTVisitor() = default;

        // Visit methods for each node type
        virtual T visitProgramNode(ProgramNode* node) = 0;
        virtual T visitBlockNode(BlockNode* node) = 0;
        virtual T visitFloatNode(FloatNode* node) = 0;
        virtual T visitIntegerNode(IntegerNode* node) = 0;
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
        virtual T visitIndexAssignmentNode(IndexAssignmentNode* node) = 0;
        virtual T visitMethodCallNode(MethodCallNode* node) = 0;
        virtual T visitMemberAccessNode(MemberAccessNode* node) = 0;
        virtual T visitNewNode(NewNode* node) = 0;
        virtual T visitMethodNode(MethodNode* node) = 0;
        virtual T visitConstructorNode(ConstructorNode* node) = 0;
        virtual T visitFieldNode(FieldNode* node) = 0;
        virtual T visitClassNode(ClassNode* node) = 0;
        virtual T visitInterfaceNode(InterfaceNode* node) = 0;
        virtual T visitArrayCreationNode(ArrayCreationNode* node) = 0;
        virtual T visitArrayLiteralNode(ArrayLiteralNode* node) = 0;
        virtual T visitIndexAccessNode(IndexAccessNode* node) = 0;
        virtual T visitLambdaNode(LambdaNode* node) = 0;
        virtual T visitForEachNode(ForEachNode* node) = 0;
        virtual T visitSuperConstructorCallNode(SuperConstructorCallNode* node) = 0;
        virtual T visitSuperMethodCallNode(SuperMethodCallNode* node) = 0;
    };
}
