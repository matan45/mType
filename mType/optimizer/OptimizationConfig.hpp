#pragma once

#include <chrono>

namespace optimizer {

	/**
	 * Configuration for optimization behavior.
	 * Controls which passes are enabled and their parameters.
	 * Always uses release-mode optimizations.
	 */
	class OptimizationConfig {
	private:
		bool enableDeadCodeElimination;
		bool enableUnusedDeclarationElimination;
		bool enableConstantFolding;
		bool enableUnreachableCodeRemoval;

		// Performance limits
		size_t maxPassIterations;
		std::chrono::milliseconds timeoutPerPass;

	public:
		OptimizationConfig();

		// Factory methods
		static OptimizationConfig forRelease();
		static OptimizationConfig noOptimization();

		// Configuration accessors
		bool isDeadCodeEliminationEnabled() const { return enableDeadCodeElimination; }
		bool isUnusedDeclarationEliminationEnabled() const { return enableUnusedDeclarationElimination; }
		bool isConstantFoldingEnabled() const { return enableConstantFolding; }
		bool isUnreachableCodeRemovalEnabled() const { return enableUnreachableCodeRemoval; }

		// Pass-specific configuration
		OptimizationConfig& setDeadCodeElimination(bool enable);
		OptimizationConfig& setUnusedDeclarationElimination(bool enable);
		OptimizationConfig& setConstantFolding(bool enable);
		OptimizationConfig& setUnreachableCodeRemoval(bool enable);

		// Performance limits
		OptimizationConfig& setMaxPassIterations(size_t iterations);
		OptimizationConfig& setTimeoutPerPass(std::chrono::milliseconds timeout);

		size_t getMaxPassIterations() const { return maxPassIterations; }
		std::chrono::milliseconds getTimeoutPerPass() const { return timeoutPerPass; }
	};

} // namespace optimizer
