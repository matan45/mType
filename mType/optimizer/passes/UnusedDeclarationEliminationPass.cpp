/**
 * Unused Declaration Elimination Pass (O2 Optimization)
 *
 * This pass removes unused functions, classes, interfaces, and variables
 * while preserving entry point code and public/exported declarations.
 *
 * Implementation Strategy:
 * 1. Phase 1: Analyze entire AST to track declarations and usages
 * 2. Compute transitive closure (if func A calls func B, and A is used, then B is used)
 * 3. Phase 2: Remove declarations that are not used
 */

#include "UnusedDeclarationEliminationPass.hpp"
#include "../base/OptimizationContext.hpp"
#include "../OptimizationResult.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/BlockNode.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../ast/nodes/statements/IfNode.hpp"
#include "../../ast/nodes/statements/WhileNode.hpp"
#include "../../ast/nodes/statements/ForNode.hpp"
#include "../../ast/nodes/statements/ForEachNode.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../ast/nodes/classes/NewNode.hpp"
#include "../../ast/nodes/classes/InterfaceNode.hpp"
#include "../../ast/nodes/expressions/VariableNode.hpp"
#include <chrono>
#include <iostream>

namespace optimizer::passes {

	using namespace ast;
	using namespace ast::nodes::statements;
	using namespace ast::nodes::functions;
	using namespace ast::nodes::classes;
	using namespace ast::nodes::expressions;

	// Debug flag
	constexpr bool UDE_DEBUG = true;

	// ================= UsageAnalyzer Implementation =================

	void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeDeclaredFunction(
		const std::string& name, bool isPublic) {
		declaredFunctions.insert(name);
		if (isPublic) {
			publicFunctions.insert(name);
			usedFunctions.insert(name); // Public = always used
		}
	}

	void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeDeclaredClass(
		const std::string& name, bool isPublic) {
		declaredClasses.insert(name);
		if (isPublic) {
			publicClasses.insert(name);
			usedClasses.insert(name); // Public = always used
		}
	}

	void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeDeclaredInterface(
		const std::string& name, bool isPublic) {
		declaredInterfaces.insert(name);
		if (isPublic) {
			publicInterfaces.insert(name);
			usedInterfaces.insert(name); // Public = always used
		}
	}

	void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeDeclaredVariable(
		const std::string& name) {
		declaredVariables.insert(name);
	}

	void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeUsedFunction(
		const std::string& name) {
		usedFunctions.insert(name);
	}

	void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeUsedClass(
		const std::string& name) {
		usedClasses.insert(name);
	}

	void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeUsedInterface(
		const std::string& name) {
		usedInterfaces.insert(name);
	}

	void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeUsedVariable(
		const std::string& name) {
		usedVariables.insert(name);
	}

	void UnusedDeclarationEliminationPass::UsageAnalyzer::computeTransitiveClosure() {
		// Compute transitive closure of function calls
		// If function A is used and calls function B, then B is also used
		bool changed = true;
		while (changed) {
			changed = false;
			for (const auto& usedFunc : usedFunctions) {
				auto it = functionCalls.find(usedFunc);
				if (it != functionCalls.end()) {
					for (const auto& calledFunc : it->second) {
						if (usedFunctions.find(calledFunc) == usedFunctions.end()) {
							usedFunctions.insert(calledFunc);
							changed = true;
						}
					}
				}
			}
		}
	}

	bool UnusedDeclarationEliminationPass::UsageAnalyzer::isFunctionUsed(
		const std::string& name) const {
		return usedFunctions.find(name) != usedFunctions.end() ||
		       publicFunctions.find(name) != publicFunctions.end();
	}

	bool UnusedDeclarationEliminationPass::UsageAnalyzer::isClassUsed(
		const std::string& name) const {
		return usedClasses.find(name) != usedClasses.end() ||
		       publicClasses.find(name) != publicClasses.end();
	}

	bool UnusedDeclarationEliminationPass::UsageAnalyzer::isInterfaceUsed(
		const std::string& name) const {
		return usedInterfaces.find(name) != usedInterfaces.end() ||
		       publicInterfaces.find(name) != publicInterfaces.end();
	}

	bool UnusedDeclarationEliminationPass::UsageAnalyzer::isVariableUsed(
		const std::string& name) const {
		return usedVariables.find(name) != usedVariables.end();
	}

	// ================= AST Analysis (Phase 1) =================

	void UnusedDeclarationEliminationPass::analyzeAST(
		ast::ASTNode* node, UsageAnalyzer& analyzer) {

		if (!node) return;

		// Track declarations
		if (auto* funcNode = dynamic_cast<FunctionNode*>(node)) {
			bool isPublic = funcNode->isPublic();
			analyzer.analyzeDeclaredFunction(funcNode->getName(), isPublic);

			// Analyze function body for usages
			analyzeAST(funcNode->getBodyPtr(), analyzer);
			return;
		}

		if (auto* classNode = dynamic_cast<ClassNode*>(node)) {
			bool isPublic = classNode->isPublic();
			analyzer.analyzeDeclaredClass(classNode->getClassName(), isPublic);

			// Classes that extend other classes use those classes
			if (!classNode->getParentClassName().empty()) {
				analyzer.analyzeUsedClass(classNode->getParentClassName());
			}

			// Classes that implement interfaces use those interfaces
			for (const auto& iface : classNode->getImplementedInterfaces()) {
				analyzer.analyzeUsedInterface(iface);
			}

			// Analyze methods
			for (const auto& method : classNode->getMethods()) {
				analyzeAST(method.get(), analyzer);
			}

			return;
		}

		if (auto* ifaceNode = dynamic_cast<InterfaceNode*>(node)) {
			bool isPublic = ifaceNode->isPublic();
			analyzer.analyzeDeclaredInterface(ifaceNode->getName(), isPublic);
			return;
		}

		if (auto* assignNode = dynamic_cast<AssignmentNode*>(node)) {
			// AssignmentNode can be either a declaration or an assignment
			// We track it as a variable either way
			analyzer.analyzeDeclaredVariable(assignNode->getVariableName());

			// Analyze value/initializer for usages
			if (assignNode->getValue()) {
				analyzeAST(assignNode->getValue(), analyzer);
			}
			return;
		}

		// Track usages
		if (auto* funcCallNode = dynamic_cast<FunctionCallNode*>(node)) {
			analyzer.analyzeUsedFunction(funcCallNode->getFunctionName());

			// Analyze arguments
			for (const auto& arg : funcCallNode->getArguments()) {
				analyzeAST(arg.get(), analyzer);
			}
			return;
		}

		if (auto* newNode = dynamic_cast<NewNode*>(node)) {
			analyzer.analyzeUsedClass(newNode->getClassName());

			// Analyze constructor arguments
			for (const auto& arg : newNode->getArguments()) {
				analyzeAST(arg.get(), analyzer);
			}
			return;
		}

		if (auto* varNode = dynamic_cast<VariableNode*>(node)) {
			analyzer.analyzeUsedVariable(varNode->getName());
			return;
		}

		// Recursively analyze children
		if (auto* programNode = dynamic_cast<ProgramNode*>(node)) {
			for (const auto& stmt : programNode->getStatements()) {
				analyzeAST(stmt.get(), analyzer);
			}
			return;
		}

		if (auto* blockNode = dynamic_cast<BlockNode*>(node)) {
			for (const auto& stmt : blockNode->getStatements()) {
				analyzeAST(stmt.get(), analyzer);
			}
			return;
		}

		if (auto* ifNode = dynamic_cast<IfNode*>(node)) {
			analyzeAST(ifNode->getCondition(), analyzer);
			analyzeAST(ifNode->getThenStatement(), analyzer);
			if (ifNode->hasElseStatement()) {
				analyzeAST(ifNode->getElseStatement(), analyzer);
			}
			return;
		}

		if (auto* whileNode = dynamic_cast<WhileNode*>(node)) {
			analyzeAST(whileNode->getCondition(), analyzer);
			analyzeAST(whileNode->getBody(), analyzer);
			return;
		}

		if (auto* forNode = dynamic_cast<ForNode*>(node)) {
			analyzeAST(forNode->getInitialization(), analyzer);
			analyzeAST(forNode->getCondition(), analyzer);
			analyzeAST(forNode->getUpdate(), analyzer);
			analyzeAST(forNode->getBody(), analyzer);
			return;
		}

		if (auto* foreachNode = dynamic_cast<ForEachNode*>(node)) {
			analyzeAST(foreachNode->getCollection(), analyzer);
			analyzeAST(foreachNode->getBody(), analyzer);
			return;
		}

		// TODO: Add more node types as needed (MethodCallNode, etc.)
	}

	// ================= Dead Declaration Removal (Phase 2) =================

	std::unique_ptr<ast::ASTNode> UnusedDeclarationEliminationPass::removeUnusedDeclarations(
		std::unique_ptr<ast::ASTNode> node,
		const UsageAnalyzer& analyzer) {

		if (!node) return nullptr;

		// ProgramNode: Filter top-level declarations
		if (auto* programNode = dynamic_cast<ProgramNode*>(node.get())) {
			std::vector<std::unique_ptr<ast::ASTNode>> keptStatements;

			for (const auto& stmt : programNode->getStatements()) {
				bool keep = true;

				// Check if this is an unused declaration
				if (auto* funcNode = dynamic_cast<FunctionNode*>(stmt.get())) {
					if (!analyzer.isFunctionUsed(funcNode->getName())) {
						keep = false;
						removedFunctions++;
						if (UDE_DEBUG) {
							std::cout << "[UDE] Removing unused function: " << funcNode->getName() << "\n";
						}
					}
				}
				else if (auto* classNode = dynamic_cast<ClassNode*>(stmt.get())) {
					if (!analyzer.isClassUsed(classNode->getClassName())) {
						keep = false;
						removedClasses++;
						if (UDE_DEBUG) {
							std::cout << "[UDE] Removing unused class: " << classNode->getClassName() << "\n";
						}
					}
				}
				else if (auto* ifaceNode = dynamic_cast<InterfaceNode*>(stmt.get())) {
					if (!analyzer.isInterfaceUsed(ifaceNode->getName())) {
						keep = false;
						removedInterfaces++;
						if (UDE_DEBUG) {
							std::cout << "[UDE] Removing unused interface: " << ifaceNode->getName() << "\n";
						}
					}
				}
				// Note: We do NOT remove top-level variable declarations/assignments
				// Top-level variables are part of the entry point code and execute immediately
				// They're not like functions/classes which are just declarations that can be unused

				if (keep) {
					// Clone the statement since we're iterating over a const reference
				keptStatements.push_back(stmt->clone());
				}
			}

			return std::make_unique<ProgramNode>(std::move(keptStatements), programNode->getLocation());
		}

		// For other nodes, just clone
		return node;
	}

	// ================= UnusedDeclarationEliminationPass Implementation =================

	UnusedDeclarationEliminationPass::UnusedDeclarationEliminationPass()
		: OptimizationPass("UnusedDeclarationElimination", base::PassType::TRANSFORMATION) {
	}

	std::unique_ptr<ast::ASTNode> UnusedDeclarationEliminationPass::optimize(
		std::unique_ptr<ast::ASTNode> node,
		base::OptimizationContext& context) {

		if (UDE_DEBUG) {
			std::cout << "[UDE] ===== Starting Unused Declaration Elimination Pass =====\n";
		}

		auto startTime = std::chrono::high_resolution_clock::now();

		// Phase 1: Analyze declarations and usages
		UsageAnalyzer analyzer;
		analyzeAST(node.get(), analyzer);

		// Compute transitive closure
		analyzer.computeTransitiveClosure();

		if (UDE_DEBUG) {
			std::cout << "[UDE] Declared functions: " << analyzer.declaredFunctions.size() << "\n";
			for (const auto& func : analyzer.declaredFunctions) {
				std::cout << "[UDE]   - " << func << "\n";
			}
			std::cout << "[UDE] Used functions: " << analyzer.usedFunctions.size() << "\n";
			for (const auto& func : analyzer.usedFunctions) {
				std::cout << "[UDE]   - " << func << "\n";
			}
			std::cout << "[UDE] Declared classes: " << analyzer.declaredClasses.size() << "\n";
			for (const auto& cls : analyzer.declaredClasses) {
				std::cout << "[UDE]   - " << cls << "\n";
			}
			std::cout << "[UDE] Used classes: " << analyzer.usedClasses.size() << "\n";
			for (const auto& cls : analyzer.usedClasses) {
				std::cout << "[UDE]   - " << cls << "\n";
			}
		}

		// Phase 2: Remove unused declarations
		auto result = removeUnusedDeclarations(std::move(node), analyzer);

		auto endTime = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
		setExecutionTime(duration);

		size_t totalRemoved = removedFunctions + removedClasses + removedInterfaces + removedVariables;

		if (totalRemoved > 0) {
			context.setModified(true);
			context.recordTransformation("UnusedDeclarationElimination");
		}

		if (UDE_DEBUG) {
			std::cout << "[UDE] Removed: " << removedFunctions << " functions, "
			          << removedClasses << " classes, "
			          << removedInterfaces << " interfaces, "
			          << removedVariables << " variables\n";
			std::cout << "[UDE] ===== Unused Declaration Elimination Pass Complete =====\n\n";
		}

		return result;
	}

	std::string UnusedDeclarationEliminationPass::getDescription() const {
		return "Removes unused functions, classes, interfaces, and variables (preserves public/entry point)";
	}

	void UnusedDeclarationEliminationPass::reportMetrics(OptimizationResult& result) const {
		size_t totalRemoved = removedFunctions + removedClasses + removedInterfaces + removedVariables;

		optimizer::PassMetrics metrics(
			getName(),
			totalRemoved,
			getExecutionTime(),
			totalRemoved > 0
		);
		result.addPassMetrics(metrics);
	}

	void UnusedDeclarationEliminationPass::reset() {
		OptimizationPass::reset();
		removedFunctions = 0;
		removedClasses = 0;
		removedInterfaces = 0;
		removedVariables = 0;
	}

} // namespace optimizer::passes
