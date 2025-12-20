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
		size_t removedMethods = 0;

		// Phase 1: Usage tracking
		struct UsageAnalyzer {
			// Declarations
			std::unordered_set<std::string> declaredFunctions;
			std::unordered_set<std::string> declaredClasses;
			std::unordered_set<std::string> declaredInterfaces;
			std::unordered_set<std::string> declaredVariables;
			std::unordered_set<std::string> declaredMethods;  // Format: "ClassName::methodName"

			// Usages
			std::unordered_set<std::string> usedFunctions;
			std::unordered_set<std::string> usedClasses;
			std::unordered_set<std::string> usedInterfaces;
			std::unordered_set<std::string> usedVariables;
			std::unordered_set<std::string> usedMethods;  // Format: "ClassName::methodName" or "ClassName::STATIC::methodName"

			// Public/exported (always keep)
			std::unordered_set<std::string> publicFunctions;
			std::unordered_set<std::string> publicClasses;
			std::unordered_set<std::string> publicInterfaces;
			std::unordered_set<std::string> publicMethods;  // Format: "ClassName::methodName" or "ClassName::STATIC::methodName"

			// Special methods that should never be removed (constructors, toString, etc.)
			std::unordered_set<std::string> specialMethods;  // Method names like "constructor", "toString"

			// Track method calls by name (without class information)
			std::unordered_set<std::string> calledMethodNames;  // Just method names: "add", "getValue", etc.

			// Track which functions contain which calls (for transitive closure)
			std::unordered_map<std::string, std::unordered_set<std::string>> functionCalls;

			// Track which interfaces each class implements (for interface propagation)
			std::unordered_map<std::string, std::unordered_set<std::string>> classImplementsInterfaces;

			// Track which interfaces extend other interfaces
			std::unordered_map<std::string, std::unordered_set<std::string>> interfaceExtendsInterfaces;

			void analyzeDeclaredFunction(const std::string& name, bool isPublic);
			void analyzeDeclaredClass(const std::string& name, bool isPublic);
			void analyzeDeclaredInterface(const std::string& name, bool isPublic);
			void analyzeDeclaredVariable(const std::string& name);
			void analyzeDeclaredMethod(const std::string& className, const std::string& methodName, bool isPublic, bool isStatic);

			void analyzeUsedFunction(const std::string& name);
			void analyzeUsedClass(const std::string& name);
			void analyzeUsedInterface(const std::string& name);
			void analyzeUsedVariable(const std::string& name);
			void analyzeUsedMethod(const std::string& className, const std::string& methodName);

			// Compute transitive closure of used declarations
			void computeTransitiveClosure();

			// Check if a declaration is used
			bool isFunctionUsed(const std::string& name) const;
			bool isClassUsed(const std::string& name) const;
			bool isInterfaceUsed(const std::string& name) const;
			bool isVariableUsed(const std::string& name) const;
			bool isMethodUsed(const std::string& className, const std::string& methodName) const;
		};

		// Analyze the entire AST to collect declarations and usages
		void analyzeAST(ast::ASTNode* node, UsageAnalyzer& analyzer);

		// Recursively analyze bodies of used functions and classes to find transitive dependencies
		void analyzeUsedDeclarations(ast::ASTNode* node, UsageAnalyzer& analyzer);

		// Phase 2: Remove unused declarations
		std::unique_ptr<ast::ASTNode> removeUnusedDeclarations(
			std::unique_ptr<ast::ASTNode> node,
			const UsageAnalyzer& analyzer
		);

		// Helper: Recursively optimize imported AST in-place
		void optimizeImportedAST(
			ast::ASTNode* importedAST,
			const UsageAnalyzer& analyzer,
			std::unordered_set<ast::ASTNode*>& optimizedASTs
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
