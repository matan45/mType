#pragma once

#include <chrono>
#include <cstddef>

namespace optimizer {

	/**
	 * Configuration for optimization behavior.
	 * Controls which passes are enabled and their parameters.
	 */
	class OptimizationConfig {
	private:
		bool enableDeadCodeElimination;
		bool enableConstantFolding;
		bool enableEscapeAnalysis;  // MYT-134
		bool enableStructuralEqualitySynthesis;  // MYT-274: auto-synthesize hashCode/equals

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
		bool isConstantFoldingEnabled() const { return enableConstantFolding; }
		bool isEscapeAnalysisEnabled() const { return enableEscapeAnalysis; }
		bool isStructuralEqualitySynthesisEnabled() const { return enableStructuralEqualitySynthesis; }

		// Pass-specific configuration
		OptimizationConfig& setDeadCodeElimination(bool enable);
		OptimizationConfig& setConstantFolding(bool enable);
		OptimizationConfig& setEscapeAnalysis(bool enable);
		OptimizationConfig& setStructuralEqualitySynthesis(bool enable);

		// Performance limits
		OptimizationConfig& setMaxPassIterations(size_t iterations);
		OptimizationConfig& setTimeoutPerPass(std::chrono::milliseconds timeout);

		size_t getMaxPassIterations() const { return maxPassIterations; }
		std::chrono::milliseconds getTimeoutPerPass() const { return timeoutPerPass; }
	};

} // namespace optimizer
