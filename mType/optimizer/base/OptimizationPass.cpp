#include "OptimizationPass.hpp"
#include "../OptimizationResult.hpp"

namespace optimizer::base {

	OptimizationPass::OptimizationPass(const std::string& name, PassType type)
		: passName(name)
		, passType(type)
		, enabled(true)
		, transformationCount(0)
		, executionTime(0) {
	}

	void OptimizationPass::reportMetrics(OptimizationResult& result) const {
		// Base implementation - can be overridden by derived classes
		// Metrics are collected and added to the result
	}

	void OptimizationPass::reset() {
		transformationCount = 0;
		executionTime = std::chrono::milliseconds(0);
	}

} // namespace optimizer::base
