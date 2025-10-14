#include "OptimizationConfig.hpp"

namespace optimizer {

	OptimizationConfig::OptimizationConfig(OptimizationLevel level)
		: level(level)
		, maxPassIterations(10)
		, timeoutPerPass(std::chrono::milliseconds(5000))
		, validateAfterEachPass(false)
		, verboseOutput(false) {

		// Configure based on optimization level
		switch (level) {
		case OptimizationLevel::O0:
			// No optimization
			enableDeadCodeElimination = false;
			enableConstantFolding = false;
			enableUnreachableCodeRemoval = false;
			break;

		case OptimizationLevel::O1:
			// Basic optimization
			enableDeadCodeElimination = true;
			enableConstantFolding = false;
			enableUnreachableCodeRemoval = false;
			break;

		case OptimizationLevel::O2:
			// Aggressive optimization
			enableDeadCodeElimination = true;
			enableConstantFolding = true;
			enableUnreachableCodeRemoval = true;
			break;

		default:
			// Default to O1
			enableDeadCodeElimination = true;
			enableConstantFolding = false;
			enableUnreachableCodeRemoval = false;
			break;
		}
	}

	OptimizationConfig OptimizationConfig::forLevel(OptimizationLevel level) {
		return OptimizationConfig(level);
	}

	OptimizationConfig OptimizationConfig::noOptimization() {
		return OptimizationConfig(OptimizationLevel::O0);
	}

	OptimizationConfig OptimizationConfig::aggressive() {
		return OptimizationConfig(OptimizationLevel::O2);
	}

	OptimizationConfig& OptimizationConfig::setDeadCodeElimination(bool enable) {
		enableDeadCodeElimination = enable;
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
