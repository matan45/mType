#pragma once

#include "../base/OptimizationPass.hpp"
#include <cstddef>
#include "../base/ASTTransformer.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include <memory>
#include <vector>

namespace optimizer::passes {

	/**
	 * Dead Code Elimination Pass
	 *
	 * Removes unreachable code after control flow terminating statements:
	 * - return statements
	 * - break statements
	 * - continue statements
	 * - throw statements
	 *
	 * Example:
	 *   function test() {
	 *       return 5;
	 *       print("unreachable");  // REMOVED
	 *       int x = 10;            // REMOVED
	 *   }
	 *
	 * Algorithm: O(n) where n = number of statements
	 * - Walk through blocks sequentially
	 * - When encountering a terminating statement, drop all subsequent statements
	 */
	class DeadCodeEliminationPass : public base::OptimizationPass {
	private:
		class DCETransformer : public base::ASTTransformer {
		private:
			size_t& removedCount;

			// Check if a statement causes control flow exit
			bool isTerminatingStatement(ast::ASTNode* node) const;

			// Transform statements by removing dead code after terminating statements
			// Returns a new vector with dead code removed
			std::vector<std::unique_ptr<ast::ASTNode>> transformStatements(
				const std::vector<std::unique_ptr<ast::ASTNode>>& statements
			);

		public:
			explicit DCETransformer(base::OptimizationContext* ctx, size_t& count);

			// Override block-level nodes to process statements
			std::unique_ptr<ast::ASTNode> visitProgramNode(ast::ProgramNode* node) override;
			std::unique_ptr<ast::ASTNode> visitBlockNode(ast::BlockNode* node) override;
			std::unique_ptr<ast::ASTNode> visitFunctionNode(ast::FunctionNode* node) override;
			std::unique_ptr<ast::ASTNode> visitMethodNode(ast::MethodNode* node) override;
			std::unique_ptr<ast::ASTNode> visitConstructorNode(ast::ConstructorNode* node) override;

			// Recursively process control flow nodes
			std::unique_ptr<ast::ASTNode> visitIfNode(ast::IfNode* node) override;
			std::unique_ptr<ast::ASTNode> visitWhileNode(ast::WhileNode* node) override;
			std::unique_ptr<ast::ASTNode> visitDoWhileNode(ast::DoWhileNode* node) override;
			std::unique_ptr<ast::ASTNode> visitForNode(ast::ForNode* node) override;
			std::unique_ptr<ast::ASTNode> visitForEachNode(ast::ForEachNode* node) override;
			std::unique_ptr<ast::ASTNode> visitSwitchNode(ast::SwitchNode* node) override;
			std::unique_ptr<ast::ASTNode> visitCaseNode(ast::CaseNode* node) override;
			std::unique_ptr<ast::ASTNode> visitDefaultCaseNode(ast::DefaultCaseNode* node) override;
			std::unique_ptr<ast::ASTNode> visitMatchNode(ast::MatchNode* node) override;
			std::unique_ptr<ast::ASTNode> visitMatchCaseNode(ast::MatchCaseNode* node) override;
			std::unique_ptr<ast::ASTNode> visitMatchDefaultNode(ast::MatchDefaultNode* node) override;
			std::unique_ptr<ast::ASTNode> visitTryNode(ast::TryNode* node) override;
			std::unique_ptr<ast::ASTNode> visitCatchNode(ast::CatchNode* node) override;
			std::unique_ptr<ast::ASTNode> visitLambdaNode(ast::LambdaNode* node) override;
			std::unique_ptr<ast::ASTNode> visitClassNode(ast::ClassNode* node) override;
			std::unique_ptr<ast::ASTNode> visitAssignmentNode(ast::AssignmentNode* node) override;
		};

		size_t removedStatements;

	public:
		DeadCodeEliminationPass();

		std::unique_ptr<ast::ASTNode> optimize(
			std::unique_ptr<ast::ASTNode> node,
			base::OptimizationContext& context
		) override;

		std::string getDescription() const override;
		void reportMetrics(OptimizationResult& result) const override;
		void reset() override;
	};

} // namespace optimizer::passes
