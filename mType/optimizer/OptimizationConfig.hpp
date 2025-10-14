#pragma once

#include <string>
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
		bool enableDeadCodeElimination;
		bool enableConstantFolding;
		bool enableUnreachableCodeRemoval;

		// Performance limits
		size_t maxPassIterations;
		std::chrono::milliseconds timeoutPerPass;

		// Debugging options
		bool validateAfterEachPass;
		bool verboseOutput;

	public:
		explicit OptimizationConfig(OptimizationLevel level = OptimizationLevel::O1);

		// Level-based presets
		static OptimizationConfig forLevel(OptimizationLevel level);
		static OptimizationConfig noOptimization();
		static OptimizationConfig aggressive();

		// Configuration accessors
		OptimizationLevel getLevel() const { return level; }
		bool isDeadCodeEliminationEnabled() const { return enableDeadCodeElimination; }
		bool isConstantFoldingEnabled() const { return enableConstantFolding; }
		bool isUnreachableCodeRemovalEnabled() const { return enableUnreachableCodeRemoval; }

		// Pass-specific configuration
		OptimizationConfig& setDeadCodeElimination(bool enable);
		OptimizationConfig& setConstantFolding(bool enable);
		OptimizationConfig& setUnreachableCodeRemoval(bool enable);

		// Performance limits
		OptimizationConfig& setMaxPassIterations(size_t iterations);
		OptimizationConfig& setTimeoutPerPass(std::chrono::milliseconds timeout);

		size_t getMaxPassIterations() const { return maxPassIterations; }
		std::chrono::milliseconds getTimeoutPerPass() const { return timeoutPerPass; }

		// Debugging
		OptimizationConfig& setValidateAfterEachPass(bool validate);
		OptimizationConfig& setVerboseOutput(bool verbose);

		bool shouldValidateAfterEachPass() const { return validateAfterEachPass; }
		bool isVerboseOutputEnabled() const { return verboseOutput; }
	};

} // namespace optimizer
