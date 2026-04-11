#pragma once

#include "../../ast/ASTNode.hpp"
#include "../../ast/ASTVisitor.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "OptimizationContext.hpp"
#include <memory>
#include <vector>

namespace optimizer::base {

	/**
	 * Base class for AST transformation passes.
	 * Returns unique_ptr<ASTNode> to support both in-place modification and
	 * creating new transformed nodes.
	 *
	 * Subclasses override visit methods for node types they want to transform.
	 * Default implementation returns nullptr, indicating no transformation.
	 *
	 * Note: Cannot inherit from ASTVisitor<unique_ptr<ASTNode>> because
	 * ASTNode::accept() is hardcoded to ASTVisitor<Value>. Uses manual dispatch instead.
	 */
	class ASTTransformer {
	protected:
		OptimizationContext* context;
		bool modified;

		// SECURITY: depth counter for transformChild() recursion. Prevents
		// crafted ASTs (or deeply nested user code that survived parser
		// limits) from blowing the C++ call stack during optimization.
		size_t transformDepth = 0;

		// Helper: Clone a child node if it needs transformation
		std::unique_ptr<ast::ASTNode> transformChild(ast::ASTNode* child);

		// Helper: Transform a vector of child nodes
		std::vector<std::unique_ptr<ast::ASTNode>> transformChildren(
			const std::vector<std::unique_ptr<ast::ASTNode>>& children
		);

	public:
		explicit ASTTransformer(OptimizationContext* ctx);
		virtual ~ASTTransformer() = default;

		// Track whether AST was modified
		bool wasModified() const { return modified; }
		void resetModified() { modified = false; }

		// Statement visitor methods - return nullptr for no transformation
		virtual std::unique_ptr<ast::ASTNode> visitProgramNode(ast::ProgramNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitBlockNode(ast::BlockNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitIfNode(ast::IfNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitWhileNode(ast::WhileNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitDoWhileNode(ast::DoWhileNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitForNode(ast::ForNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitForEachNode(ast::ForEachNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitBreakNode(ast::BreakNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitContinueNode(ast::ContinueNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitReturnNode(ast::ReturnNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitSwitchNode(ast::SwitchNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitCaseNode(ast::CaseNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitDefaultCaseNode(ast::DefaultCaseNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitMatchNode(ast::MatchNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitMatchCaseNode(ast::MatchCaseNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitMatchDefaultNode(ast::MatchDefaultNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitAssignmentNode(ast::AssignmentNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitMemberAssignmentNode(ast::MemberAssignmentNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitIndexAssignmentNode(ast::IndexAssignmentNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitImportNode(ast::ImportNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitTryNode(ast::TryNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitCatchNode(ast::CatchNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitThrowNode(ast::ThrowNode* node);

		// Expression visitor methods
		virtual std::unique_ptr<ast::ASTNode> visitBinaryOpNode(ast::BinaryOpNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitTernaryOpNode(ast::TernaryOpNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitUnaryOpNode(ast::UnaryOpNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitIntegerNode(ast::IntegerNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitFloatNode(ast::FloatNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitStringNode(ast::StringNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitBoolNode(ast::BoolNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitNullNode(ast::NullNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitVariableNode(ast::VariableNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitArrayLiteralNode(ast::ArrayLiteralNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitArrayCreationNode(ast::ArrayCreationNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitIndexAccessNode(ast::IndexAccessNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitLambdaNode(ast::LambdaNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitCastExpression(ast::CastExpression* node);
		virtual std::unique_ptr<ast::ASTNode> visitInstanceOfExpression(ast::InstanceOfExpression* node);
		virtual std::unique_ptr<ast::ASTNode> visitAwaitExpression(ast::AwaitExpression* node);

		// Function visitor methods
		virtual std::unique_ptr<ast::ASTNode> visitFunctionNode(ast::FunctionNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitFunctionCallNode(ast::FunctionCallNode* node);

		// Class visitor methods
		virtual std::unique_ptr<ast::ASTNode> visitClassNode(ast::ClassNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitFieldNode(ast::FieldNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitMethodNode(ast::MethodNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitConstructorNode(ast::ConstructorNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitNewNode(ast::NewNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitMemberAccessNode(ast::MemberAccessNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitMethodCallNode(ast::MethodCallNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitInterfaceNode(ast::InterfaceNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitSuperConstructorCallNode(ast::SuperConstructorCallNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitSuperMethodCallNode(ast::SuperMethodCallNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitSuperMemberAccessNode(ast::SuperMemberAccessNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitSuperMemberAssignmentNode(ast::SuperMemberAssignmentNode* node);
		virtual std::unique_ptr<ast::ASTNode> visitThisConstructorCallNode(ast::ThisConstructorCallNode* node);
	};

} // namespace optimizer::base
