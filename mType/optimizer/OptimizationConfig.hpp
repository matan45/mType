#pragma once

#include <chrono>
#include "../constants/ExecutionMode.hpp"

namespace optimizer {

	using constants::OptimizationLevel;

	/**
	 * Configuration for optimization behavior.
	 * Controls which passes are enabled and their parameters.
	 */
	class OptimizationConfig {
	private:
		OptimizationLevel level;
		bool enableDeadCodeElimination;  // Release: Unreachable code after return/break/continue/throw
		bool enableUnusedDeclarationElimination;  // Release: Remove unused functions/classes/variables
		bool enableConstantFolding;
		bool enableUnreachableCodeRemoval;

		// Performance limits
		size_t maxPassIterations;
		std::chrono::milliseconds timeoutPerPass;

	public:
		explicit OptimizationConfig(OptimizationLevel level = OptimizationLevel::Debug);

		// Level-based presets
		static OptimizationConfig forLevel(OptimizationLevel level);
		static OptimizationConfig noOptimization();
		static OptimizationConfig aggressive();

		// Configuration accessors
		OptimizationLevel getLevel() const { return level; }
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
