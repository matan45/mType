#pragma once

#include "../base/OptimizationPass.hpp"
#include "../base/ASTTransformer.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace optimizer::passes {

	/**
	 * Abstract Class Validation Pass
	 *
	 * Validates abstract class usage during optimization:
	 * - Detects attempts to instantiate abstract classes
	 * - Validates concrete classes implement all abstract methods
	 * - Validates abstract method declarations
	 *
	 * Example validations:
	 *   abstract class Shape {
	 *       abstract function getArea(): float;
	 *   }
	 *   Shape s = new Shape();  // ERROR: Cannot instantiate abstract class
	 *
	 * Algorithm: O(n) where n = number of AST nodes
	 * - Walk through all ClassNodes and NewNodes
	 * - Validate abstract class constraints
	 */
	class AbstractClassValidationPass : public base::OptimizationPass {
	private:
		class AbstractValidationTransformer : public base::ASTTransformer {
		private:
			size_t& validationCount;
			std::vector<std::string>& warnings;

			// Track abstract classes and their abstract methods
			std::unordered_map<std::string, std::unordered_set<std::string>> abstractClasses;
			std::unordered_map<std::string, std::string> classParents; // child -> parent

			// Check if a class is abstract
			bool isAbstractClass(const std::string& className) const;

			// Validate NewNode doesn't instantiate abstract class
			void validateNewNode(ast::NewNode* node);

			// Validate ClassNode abstract constraints
			void validateClassNode(ast::ClassNode* node);

			// Collect abstract methods from a class
			std::unordered_set<std::string> collectAbstractMethods(ast::ClassNode* node);

		public:
			explicit AbstractValidationTransformer(base::OptimizationContext* ctx, size_t& count, std::vector<std::string>& warnings);

			// Override to collect class information
			std::unique_ptr<ast::ASTNode> visitClassNode(ast::ClassNode* node) override;

			// Override to validate instantiation
			std::unique_ptr<ast::ASTNode> visitNewNode(ast::NewNode* node) override;

			// Process program to build class hierarchy first
			std::unique_ptr<ast::ASTNode> visitProgramNode(ast::ProgramNode* node) override;
		};

		size_t validationsPassed;
		std::vector<std::string> validationWarnings;

	public:
		AbstractClassValidationPass();

		std::unique_ptr<ast::ASTNode> optimize(
			std::unique_ptr<ast::ASTNode> node,
			base::OptimizationContext& context
		) override;

		std::string getDescription() const override;
		void reportMetrics(OptimizationResult& result) const override;
		void reset() override;

		base::PassType getType() const override { return base::PassType::VALIDATION; }
	};

} // namespace optimizer::passes
