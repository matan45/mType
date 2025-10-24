#include "OptimizationConfig.hpp"

namespace optimizer {

	OptimizationConfig::OptimizationConfig(OptimizationLevel level)
		: level(level)
		, maxPassIterations(50)
		, timeoutPerPass(std::chrono::milliseconds(5000))
		, validateAfterEachPass(false)
		, verboseOutput(false) {

		// Configure based on optimization level
		switch (level) {
		case OptimizationLevel::Debug:
			// Debug mode - no dead code passes
			enableDeadCodeElimination = false;
			enableUnusedDeclarationElimination = false;
			enableConstantFolding = false;
			enableUnreachableCodeRemoval = false;
			verboseOutput = false;  // No verbose output in debug mode
			break;

		case OptimizationLevel::Release:
			// Release mode - full optimization including dead code elimination
			enableDeadCodeElimination = true;
			enableUnusedDeclarationElimination = true;
			enableConstantFolding = true;
			enableUnreachableCodeRemoval = true;
			verboseOutput = true;  // Disabled verbose output
			break;

		default:
			// Default to Debug
			enableDeadCodeElimination = false;
			enableUnusedDeclarationElimination = false;
			enableConstantFolding = false;
			enableUnreachableCodeRemoval = false;
			verboseOutput = false;
			break;
		}
	}

	OptimizationConfig OptimizationConfig::forLevel(OptimizationLevel level) {
		return OptimizationConfig(level);
	}

	OptimizationConfig OptimizationConfig::noOptimization() {
		return OptimizationConfig(OptimizationLevel::Debug);
	}

	OptimizationConfig OptimizationConfig::aggressive() {
		return OptimizationConfig(OptimizationLevel::Release);
	}

	OptimizationConfig& OptimizationConfig::setDeadCodeElimination(bool enable) {
		enableDeadCodeElimination = enable;
		return *this;
	}

	OptimizationConfig& OptimizationConfig::setUnusedDeclarationElimination(bool enable) {
		enableUnusedDeclarationElimination = enable;
		return *this;
	}

	OptimizationConfig& OptimizationConfig::setConstantFolding(bool enable) {
		enableConstantFolding = enable;
		return *this;
	}

	OptimizationConfig& OptimizationConfig::setUnreachableCodeRemoval(bool enable) {
		enableUnreachableCodeRemoval = enable;
		return *this;
	}

	OptimizationConfig& OptimizationConfig::setMaxPassIterations(size_t iterations) {
		maxPassIterations = iterations;
		return *this;
	}

	OptimizationConfig& OptimizationConfig::setTimeoutPerPass(std::chrono::milliseconds timeout) {
		timeoutPerPass = timeout;
		return *this;
	}

	OptimizationConfig& OptimizationConfig::setValidateAfterEachPass(bool validate) {
		validateAfterEachPass = validate;
		return *this;
	}

	OptimizationConfig& OptimizationConfig::setVerboseOutput(bool verbose) {
		verboseOutput = verbose;
		return *this;
	}

} // namespace optimizer
