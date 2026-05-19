#include "BytecodeCompiler.hpp"
#include "../../ast/nodes/expressions/AwaitExpression.hpp"
#include "../bytecode/OpCode.hpp"
#include <stdexcept>

namespace vm::compiler
{
    value::Value BytecodeCompiler::visitProgramNode(ast::ProgramNode* node)
    {
        return statementCompiler.compileProgram(node);
    }

    value::Value BytecodeCompiler::visitBlockNode(ast::BlockNode* node)
    {
        return statementCompiler.compileBlock(node);
    }

    value::Value BytecodeCompiler::visitIntegerNode(ast::IntegerNode* node)
    {
        return literalCompiler.compileInteger(node);
    }

    value::Value BytecodeCompiler::visitFloatNode(ast::FloatNode* node)
    {
        return literalCompiler.compileFloat(node);
    }

    value::Value BytecodeCompiler::visitStringNode(ast::StringNode* node)
    {
        return literalCompiler.compileString(node);
    }

    value::Value BytecodeCompiler::visitBoolNode(ast::BoolNode* node)
    {
        return literalCompiler.compileBool(node);
    }

    value::Value BytecodeCompiler::visitNullNode(ast::NullNode* node)
    {
        return literalCompiler.compileNull(node);
    }

    value::Value BytecodeCompiler::visitVariableNode(ast::VariableNode* node)
    {
        return expressionCompiler.compileVariable(node);
    }

    value::Value BytecodeCompiler::visitAssignmentNode(ast::AssignmentNode* node)
    {
        return statementCompiler.compileAssignment(node);
    }

    value::Value BytecodeCompiler::visitBinaryOpNode(ast::BinaryOpNode* node)
    {
        return expressionCompiler.compileBinaryOp(node);
    }

    value::Value BytecodeCompiler::visitUnaryOpNode(ast::UnaryOpNode* node)
    {
        return expressionCompiler.compileUnaryOp(node);
    }

    value::Value BytecodeCompiler::visitTernaryOpNode(ast::TernaryOpNode* node)
    {
        return expressionCompiler.compileTernaryOp(node);
    }

    value::Value BytecodeCompiler::visitIfNode(ast::IfNode* node)
    {
        return controlFlowCompiler.compileIf(node);
    }

    value::Value BytecodeCompiler::visitWhileNode(ast::WhileNode* node)
    {
        return controlFlowCompiler.compileWhile(node);
    }

    value::Value BytecodeCompiler::visitDoWhileNode(ast::DoWhileNode* node)
    {
        return controlFlowCompiler.compileDoWhile(node);
    }

    value::Value BytecodeCompiler::visitForNode(ast::ForNode* node)
    {
        return controlFlowCompiler.compileFor(node);
    }

    value::Value BytecodeCompiler::visitForEachNode(ast::ForEachNode* node)
    {
        return controlFlowCompiler.compileForEach(node);
    }

    value::Value BytecodeCompiler::visitBreakNode(ast::BreakNode* node)
    {
        return controlFlowCompiler.compileBreak(node);
    }

    value::Value BytecodeCompiler::visitContinueNode(ast::ContinueNode* node)
    {
        return controlFlowCompiler.compileContinue(node);
    }

    value::Value BytecodeCompiler::visitSwitchNode(ast::SwitchNode* node)
    {
        return controlFlowCompiler.compileSwitch(node);
    }

    value::Value BytecodeCompiler::visitCaseNode(ast::CaseNode* node)
    {
        return controlFlowCompiler.compileCase(node);
    }

    value::Value BytecodeCompiler::visitDefaultCaseNode(ast::DefaultCaseNode* node)
    {
        return controlFlowCompiler.compileDefaultCase(node);
    }

    value::Value BytecodeCompiler::visitMatchNode(ast::MatchNode* node)
    {
        return controlFlowCompiler.compileMatch(node);
    }

    value::Value BytecodeCompiler::visitMatchCaseNode(ast::MatchCaseNode* node)
    {
        // Handled inline by compileMatch
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitMatchDefaultNode(ast::MatchDefaultNode* node)
    {
        // Handled inline by compileMatch
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitFunctionNode(ast::FunctionNode* node)
    {
        return functionCompiler.compileFunction(node);
    }

    value::Value BytecodeCompiler::visitFunctionCallNode(ast::FunctionCallNode* node)
    {
        return functionCompiler.compileFunctionCall(node);
    }

    value::Value BytecodeCompiler::visitReturnNode(ast::ReturnNode* node)
    {
        return functionCompiler.compileReturn(node);
    }

    value::Value BytecodeCompiler::visitLambdaNode(ast::LambdaNode* node)
    {
        return functionCompiler.compileLambda(node);
    }

    value::Value BytecodeCompiler::visitClassNode(ast::ClassNode* node)
    {
        return classCompiler.compileClass(node);
    }

    value::Value BytecodeCompiler::visitMethodNode(ast::MethodNode* node)
    {
        return classCompiler.compileMethod(node);
    }

    value::Value BytecodeCompiler::visitConstructorNode(ast::ConstructorNode* node)
    {
        return classCompiler.compileConstructor(node);
    }

    value::Value BytecodeCompiler::visitFieldNode(ast::FieldNode* node)
    {
        return classCompiler.compileField(node);
    }

    value::Value BytecodeCompiler::visitNewNode(ast::NewNode* node)
    {
        return classCompiler.compileNew(node);
    }

    value::Value BytecodeCompiler::visitMemberAccessNode(ast::MemberAccessNode* node)
    {
        return classCompiler.compileMemberAccess(node);
    }

    value::Value BytecodeCompiler::visitMemberAssignmentNode(ast::MemberAssignmentNode* node)
    {
        return classCompiler.compileMemberAssignment(node);
    }

    value::Value BytecodeCompiler::visitMethodCallNode(ast::MethodCallNode* node)
    {
        return classCompiler.compileMethodCall(node);
    }

    value::Value BytecodeCompiler::visitSuperConstructorCallNode(ast::SuperConstructorCallNode* node)
    {
        return classCompiler.compileSuperConstructorCall(node);
    }

    value::Value BytecodeCompiler::visitThisConstructorCallNode(ast::ThisConstructorCallNode* node)
    {
        return classCompiler.compileThisConstructorCall(node);
    }

    value::Value BytecodeCompiler::visitSuperMethodCallNode(ast::SuperMethodCallNode* node)
    {
        return classCompiler.compileSuperMethodCall(node);
    }

    value::Value BytecodeCompiler::visitSuperMemberAccessNode(ast::SuperMemberAccessNode* node)
    {
        return classCompiler.compileSuperMemberAccess(node);
    }

    value::Value BytecodeCompiler::visitSuperMemberAssignmentNode(ast::SuperMemberAssignmentNode* node)
    {
        return classCompiler.compileSuperMemberAssignment(node);
    }

    value::Value BytecodeCompiler::visitInterfaceNode(ast::InterfaceNode* node)
    {
        // Interfaces are registered during registerClassesForBytecode phase
        // No bytecode needs to be generated for interface declarations
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitArrayCreationNode(ast::ArrayCreationNode* node)
    {
        return arrayCompiler.compileArrayCreation(node);
    }

    value::Value BytecodeCompiler::visitArrayLiteralNode(ast::ArrayLiteralNode* node)
    {
        return arrayCompiler.compileArrayLiteral(node);
    }

    value::Value BytecodeCompiler::visitIndexAccessNode(ast::IndexAccessNode* node)
    {
        return arrayCompiler.compileIndexAccess(node);
    }

    value::Value BytecodeCompiler::visitIndexAssignmentNode(ast::IndexAssignmentNode* node)
    {
        return arrayCompiler.compileIndexAssignment(node);
    }

    value::Value BytecodeCompiler::visitCastExpression(ast::CastExpression* node)
    {
        return expressionCompiler.compileCast(node);
    }

    value::Value BytecodeCompiler::visitInstanceOfExpression(ast::InstanceOfExpression* node)
    {
        return expressionCompiler.compileInstanceOf(node);
    }

    value::Value BytecodeCompiler::visitAwaitExpression(ast::AwaitExpression* node)
    {
        // Compile the expression being awaited (should evaluate to a Promise)
        node->getExpressionPtr()->accept(*this);

        // Emit AWAIT to unwrap the Promise. In the Phase 2 synchronous model,
        // this immediately returns the Promise's value.
        context.emitter.emitWithLocation(bytecode::OpCode::AWAIT, node);

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitTryNode(ast::TryNode* node)
    {
        return controlFlowCompiler.compileTry(node);
    }

    value::Value BytecodeCompiler::visitCatchNode(ast::CatchNode* node)
    {
        // Catch nodes should not be visited directly
        throw std::runtime_error("Catch nodes should only be processed within try statements");
    }

    value::Value BytecodeCompiler::visitThrowNode(ast::ThrowNode* node)
    {
        return controlFlowCompiler.compileThrow(node);
    }

    value::Value BytecodeCompiler::visitAnnotationNode(ast::AnnotationNode* node)
    {
        // Annotations are metadata only - processed during semantic analysis,
        // no bytecode generation needed here.
        return value::Value();
    }
}
