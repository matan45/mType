#pragma once

#include "../../value/ValueType.hpp"
#include <cstddef>
#include "../base/OptimizationPass.hpp"
#include "../base/ASTTransformer.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include <memory>
#include <optional>

namespace optimizer::passes {

	/**
	 * Constant Folding Pass
	 *
	 * Evaluates and folds constant expressions at compile-time to improve runtime performance.
	 * Replaces complex expressions with their constant results when all operands are literals.
	 *
	 * Examples:
	 *   5 + 3          -> 8
	 *   10 / 2         -> 5
	 *   true && false  -> false
	 *   "hello" + " world" -> "hello world"
	 *   2.5 * 4.0      -> 10.0
	 *   -(5)           -> -5
	 *   !true          -> false
	 *   5 > 3          -> true
	 *   (int)3.14      -> 3
	 *   true ? 5 : 10  -> 5
	 *
	 * Operations Supported:
	 * - Arithmetic: +, -, *, /, % (with overflow/underflow detection)
	 * - Comparison: ==, !=, <, >, <=, >=
	 * - Logical: &&, ||, ! (with short-circuit optimization)
	 * - Unary: -, +, !
	 * - String: Concatenation (+)
	 * - Ternary: condition ? true_expr : false_expr
	 * - Type Casts: Primitive type conversions
	 *
	 * Safety Features:
	 * - Integer overflow/underflow detection (conservative - don't fold on overflow)
	 * - Division by zero checks (preserve runtime error)
	 * - Floating-point NaN/Infinity checks (conservative mode)
	 * - Type safety validation before folding
	 *
	 * Algorithm: O(n) where n = number of AST nodes
	 * - Bottom-up traversal (fold leaves first, then parents)
	 * - Conservative approach: when in doubt, don't fold
	 * - Preserves source location for error reporting
	 */
	class ConstantFoldingPass : public base::OptimizationPass {
	private:
		class CFTransformer : public base::ASTTransformer {
		private:
			size_t& foldedExpressionsCount;

			// Helper methods (implementation in .cpp to avoid Value type exposure)
			std::optional<value::Value> extractLiteralValue(ast::ASTNode* node);
			std::unique_ptr<ast::ASTNode> createLiteralNode(const value::Value& value, const errors::SourceLocation& loc);

			// Try to fold a binary operation node
			std::unique_ptr<ast::ASTNode> tryFoldBinaryOp(ast::BinaryOpNode* node);

			// Try to fold a unary operation node
			std::unique_ptr<ast::ASTNode> tryFoldUnaryOp(ast::UnaryOpNode* node);

			// Try to fold a ternary conditional node
			std::unique_ptr<ast::ASTNode> tryFoldTernary(ast::TernaryOpNode* node);

			// Try to fold a type cast expression
			std::unique_ptr<ast::ASTNode> tryFoldCast(ast::CastExpression* node);

			// Check if node is a literal (can be extracted)
			bool isLiteralNode(ast::ASTNode* node) const;

		public:
			explicit CFTransformer(base::OptimizationContext* ctx, size_t& count);

			// Override expression nodes that can be folded
			std::unique_ptr<ast::ASTNode> visitBinaryOpNode(ast::BinaryOpNode* node) override;
			std::unique_ptr<ast::ASTNode> visitUnaryOpNode(ast::UnaryOpNode* node) override;
			std::unique_ptr<ast::ASTNode> visitTernaryOpNode(ast::TernaryOpNode* node) override;
			std::unique_ptr<ast::ASTNode> visitCastExpression(ast::CastExpression* node) override;

			// Recursively transform container nodes
			std::unique_ptr<ast::ASTNode> visitProgramNode(ast::ProgramNode* node) override;
			std::unique_ptr<ast::ASTNode> visitBlockNode(ast::BlockNode* node) override;
			std::unique_ptr<ast::ASTNode> visitFunctionNode(ast::FunctionNode* node) override;
			std::unique_ptr<ast::ASTNode> visitMethodNode(ast::MethodNode* node) override;
			std::unique_ptr<ast::ASTNode> visitConstructorNode(ast::ConstructorNode* node) override;
			std::unique_ptr<ast::ASTNode> visitIfNode(ast::IfNode* node) override;
			std::unique_ptr<ast::ASTNode> visitWhileNode(ast::WhileNode* node) override;
			std::unique_ptr<ast::ASTNode> visitForNode(ast::ForNode* node) override;
			std::unique_ptr<ast::ASTNode> visitAssignmentNode(ast::AssignmentNode* node) override;
			std::unique_ptr<ast::ASTNode> visitReturnNode(ast::ReturnNode* node) override;
			std::unique_ptr<ast::ASTNode> visitClassNode(ast::ClassNode* node) override;
			std::unique_ptr<ast::ASTNode> visitArrayLiteralNode(ast::ArrayLiteralNode* node) override;
		};

		size_t foldedExpressions;

	public:
		ConstantFoldingPass();

		std::unique_ptr<ast::ASTNode> optimize(
			std::unique_ptr<ast::ASTNode> node,
			base::OptimizationContext& context
		) override;

		std::string getDescription() const override;
		void reportMetrics(OptimizationResult& result) const override;
		void reset() override;
	};

} // namespace optimizer::passes
