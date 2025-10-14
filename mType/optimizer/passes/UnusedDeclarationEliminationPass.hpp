#pragma once

#include "../base/OptimizationPass.hpp"
#include "../../ast/ASTNode.hpp"
#include "../../ast/ASTVisitor.hpp"
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <memory>

namespace optimizer::passes {

	/**
	 * Unused Declaration Elimination Pass (O2 Optimization)
	 *
	 * Two-Phase Approach:
	 *
	 * Phase 1 - Usage Analysis:
	 *   Traverses the entire AST to collect:
	 *   - All declarations (functions, classes, interfaces, variables)
	 *   - All usages (function calls, class instantiations, variable references)
	 *
	 * Phase 2 - Dead Declaration Removal:
	 *   Removes unused declarations while preserving:
	 *   - Entry point code (top-level statements)
	 *   - Public/exported declarations
	 *   - Declarations used by kept code
	 *
	 * What gets tracked:
	 *   - Functions: FunctionNode → FunctionCallNode
	 *   - Classes: ClassNode → NewNode, extends, type annotations
	 *   - Interfaces: InterfaceNode → implements, type annotations
	 *   - Variables: VariableDeclarationNode → VariableNode
	 *   - Methods: MethodNode → method calls on objects
	 */
	class UnusedDeclarationEliminationPass : public base::OptimizationPass {
	private:
		// Statistics
		size_t removedFunctions = 0;
		size_t removedClasses = 0;
		size_t removedInterfaces = 0;
		size_t removedVariables = 0;

		// Phase 1: Usage tracking
		struct UsageAnalyzer {
			// Declarations
			std::unordered_set<std::string> declaredFunctions;
			std::unordered_set<std::string> declaredClasses;
			std::unordered_set<std::string> declaredInterfaces;
			std::unordered_set<std::string> declaredVariables;

			// Usages
			std::unordered_set<std::string> usedFunctions;
			std::unordered_set<std::string> usedClasses;
			std::unordered_set<std::string> usedInterfaces;
			std::unordered_set<std::string> usedVariables;

			// Public/exported (always keep)
			std::unordered_set<std::string> publicFunctions;
			std::unordered_set<std::string> publicClasses;
			std::unordered_set<std::string> publicInterfaces;

			// Track which functions contain which calls (for transitive closure)
			std::unordered_map<std::string, std::unordered_set<std::string>> functionCalls;

			void analyzeDeclaredFunction(const std::string& name, bool isPublic);
			void analyzeDeclaredClass(const std::string& name, bool isPublic);
			void analyzeDeclaredInterface(const std::string& name, bool isPublic);
			void analyzeDeclaredVariable(const std::string& name);

			void analyzeUsedFunction(const std::string& name);
			void analyzeUsedClass(const std::string& name);
			void analyzeUsedInterface(const std::string& name);
			void analyzeUsedVariable(const std::string& name);

			// Compute transitive closure of used declarations
			void computeTransitiveClosure();

			// Check if a declaration is used
			bool isFunctionUsed(const std::string& name) const;
			bool isClassUsed(const std::string& name) const;
			bool isInterfaceUsed(const std::string& name) const;
			bool isVariableUsed(const std::string& name) const;
		};

		// Analyze the entire AST to collect declarations and usages
		void analyzeAST(ast::ASTNode* node, UsageAnalyzer& analyzer);

		// Phase 2: Remove unused declarations
		std::unique_ptr<ast::ASTNode> removeUnusedDeclarations(
			std::unique_ptr<ast::ASTNode> node,
			const UsageAnalyzer& analyzer
		);

	public:
		UnusedDeclarationEliminationPass();

		std::unique_ptr<ast::ASTNode> optimize(
			std::unique_ptr<ast::ASTNode> node,
			base::OptimizationContext& context
		) override;

		std::string getDescription() const override;
		void reportMetrics(OptimizationResult& result) const override;
		void reset() override;
	};

} // namespace optimizer::passes
