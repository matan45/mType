#pragma once

namespace ast
{
       namespace nodes
    {
        namespace expressions
        {
            class BinaryExpNode;
            class TernaryExpNode;
            class UnaryExpNode;
            class NumberNode;
            class StringNode;
            class BoolNode;
            class NullNode;
            class VariableNode;
            class IntegerNode;
            class FloatNode;
        }
        
        namespace statements
        {
            class ProgramNode;
            class BlockNode;
            class DeclarationNode;
            class ImportNode;
            class NativeFunctionNode;
            class AssignmentNode;
            class QualifiedAssignmentNode;
            class MemberAssignmentNode;
            class IfNode;
            class WhileNode;
            class DoWhileNode;
            class ForNode;
            class BreakNode;
            class ContinueNode;
            class SwitchNode;
            class CaseNode;
            class DefaultCaseNode;
        }
        
        namespace functions
        {
            class FunctionNode;
            class FunctionCallNode;
            class ReturnNode;
        }
        
        namespace classes
        {
            class ClassNode;
            class MethodNode;
            class FieldNode;
            class ConstructorNode;
            class NewNode;
            class MemberAccessNode;
            class MethodCallNode;
        }
    }
    
    // Type aliases for backward compatibility - these map to the refactored namespaced types
    using ProgramNode = nodes::statements::ProgramNode;
    using BlockNode = nodes::statements::BlockNode;
    using StringNode = nodes::expressions::StringNode;
    using BoolNode = nodes::expressions::BoolNode;
    using VariableNode = nodes::expressions::VariableNode;
    using DeclarationNode = nodes::statements::DeclarationNode;
    using AssignmentNode = nodes::statements::AssignmentNode;
    using BinaryOpNode = nodes::expressions::BinaryExpNode;
    using TernaryOpNode = nodes::expressions::TernaryExpNode;
    using UnaryOpNode = nodes::expressions::UnaryExpNode;
    using IfNode = nodes::statements::IfNode;
    using WhileNode = nodes::statements::WhileNode;
    using DoWhileNode = nodes::statements::DoWhileNode;
    using ForNode = nodes::statements::ForNode;
    using BreakNode = nodes::statements::BreakNode;
    using ContinueNode = nodes::statements::ContinueNode;
    using FunctionNode = nodes::functions::FunctionNode;
    using FunctionCallNode = nodes::functions::FunctionCallNode;
    using ReturnNode = nodes::functions::ReturnNode;
    using SwitchNode = nodes::statements::SwitchNode;
    using CaseNode = nodes::statements::CaseNode;
    using DefaultCaseNode = nodes::statements::DefaultCaseNode;
    using NativeFunctionNode = nodes::statements::NativeFunctionNode;
    using ImportNode = nodes::statements::ImportNode;
    using ClassNode = nodes::classes::ClassNode;
    using FieldNode = nodes::classes::FieldNode;
    using ConstructorNode = nodes::classes::ConstructorNode;
    using MethodNode = nodes::classes::MethodNode;
    using NewNode = nodes::classes::NewNode;
    using MemberAccessNode = nodes::classes::MemberAccessNode;
    using MethodCallNode = nodes::classes::MethodCallNode;
    using MemberAssignmentNode = nodes::statements::MemberAssignmentNode;
    using NullNode = nodes::expressions::NullNode;
    using FloatNode = nodes::expressions::FloatNode;
    using IntegerNode = nodes::expressions::IntegerNode;

    // Abstract visitor interface
    template<typename T>
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
        virtual T visitMethodCallNode(MethodCallNode* node) = 0;
        virtual T visitMemberAccessNode(MemberAccessNode* node) = 0;
        virtual T visitNewNode(NewNode* node) = 0;
        virtual T visitMethodNode(MethodNode* node) = 0;
        virtual T visitConstructorNode(ConstructorNode* node) = 0;
        virtual T visitFieldNode(FieldNode* node) = 0;
        virtual T visitClassNode(ClassNode* node) = 0;
    };
}