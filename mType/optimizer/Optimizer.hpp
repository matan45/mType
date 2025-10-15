#pragma once

#include "OptimizationConfig.hpp"
#include "OptimizationResult.hpp"
#include "OptimizationPassManager.hpp"
#include "base/OptimizationContext.hpp"
#include "../ast/ASTNode.hpp"
#include "../environment/Environment.hpp"
#include <memory>

namespace optimizer {

	/**
	 * Main entry point for AST optimization.
	 * Provides a simple API for optimizing AST nodes.
	 *
	 * Follows Facade Pattern - simplifies complex optimization subsystem
	 * Follows Single Responsibility - coordinates optimization process
	 *
	 * Usage:
	 *   auto config = OptimizationConfig::forLevel(OptimizationLevel::Release);
	 *   Optimizer optimizer(config);
	 *   ast = optimizer.optimize(std::move(ast), environment);
	 */
	class Optimizer {
	private:
		std::unique_ptr<OptimizationPassManager> passManager;
		OptimizationConfig config;

	public:
		explicit Optimizer(const OptimizationConfig& cfg = OptimizationConfig());
		~Optimizer() = default;

		// Main optimization API
		std::unique_ptr<ast::ASTNode> optimize(
			std::unique_ptr<ast::ASTNode> ast,
			std::shared_ptr<environment::Environment> environment
		);

		// Configuration
		void setConfig(const OptimizationConfig& cfg);
		const OptimizationConfig& getConfig() const { return config; }

		// Pass management
		void enablePass(const std::string& passName);
		void disablePass(const std::string& passName);

		// Results
		OptimizationResult getLastResult() const;

		// Utility - count total nodes in AST
		size_t countASTNodes(const ast::ASTNode* node) const;
	};

} // namespace optimizer
