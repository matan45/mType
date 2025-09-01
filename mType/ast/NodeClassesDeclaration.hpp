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
}