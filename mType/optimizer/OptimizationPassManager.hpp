#pragma once

#include "OptimizationConfig.hpp"
#include "OptimizationResult.hpp"
#include "base/OptimizationPass.hpp"
#include "base/OptimizationContext.hpp"
#include <memory>
#include <vector>
#include <string>

namespace optimizer {

	/**
	 * Orchestrates execution of multiple optimization passes.
	 * Manages pass ordering, dependency resolution, and execution.
	 *
	 * Follows Strategy Pattern - different optimization strategies via registered passes
	 * Follows Chain of Responsibility - passes process AST sequentially
	 */
	class OptimizationPassManager {
	private:
		std::vector<std::unique_ptr<base::OptimizationPass>> passes;
		OptimizationConfig config;
		OptimizationResult lastResult;

		// Check if pass dependencies are satisfied
		bool checkDependenciesSatisfied(base::OptimizationPass* pass) const;

	public:
		explicit OptimizationPassManager(const OptimizationConfig& cfg);
		~OptimizationPassManager() = default;

		// Pass registration
		void registerPass(std::unique_ptr<base::OptimizationPass> pass);
		void registerDefaultPasses();

		// Execution
		std::unique_ptr<ast::ASTNode> runPasses(
			std::unique_ptr<ast::ASTNode> ast,
			base::OptimizationContext& context
		);

		// Configuration
		void setConfig(const OptimizationConfig& cfg);
		const OptimizationConfig& getConfig() const { return config; }

		// Pass management
		void enablePass(const std::string& passName);
		void disablePass(const std::string& passName);
		base::OptimizationPass* getPass(const std::string& passName);
		std::vector<std::string> getRegisteredPasses() const;

		// Metrics and reporting
		const OptimizationResult& getLastResult() const { return lastResult; }
	};

} // namespace optimizer
