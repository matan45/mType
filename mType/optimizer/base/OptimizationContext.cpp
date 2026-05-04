#include "OptimizationContext.hpp"
#include <cstddef>

namespace optimizer::base {

	OptimizationContext::OptimizationContext(
		std::shared_ptr<environment::Environment> env,
		const OptimizationConfig& cfg)
		: environment(env)
		, config(cfg)
		, modified(false) {
	}

	void OptimizationContext::markVariableUsed(const std::string& name) {
		usedVariables.insert(name);
	}

	void OptimizationContext::markFunctionUsed(const std::string& name) {
		usedFunctions.insert(name);
	}

	void OptimizationContext::markClassUsed(const std::string& name) {
		usedClasses.insert(name);
	}

	bool OptimizationContext::isVariableUsed(const std::string& name) const {
		return usedVariables.find(name) != usedVariables.end();
	}

	bool OptimizationContext::isFunctionUsed(const std::string& name) const {
		return usedFunctions.find(name) != usedFunctions.end();
	}

	bool OptimizationContext::isClassUsed(const std::string& name) const {
		return usedClasses.find(name) != usedClasses.end();
	}

	void OptimizationContext::recordTransformation(const std::string& passName) {
		++transformationCounts[passName];
		modified = true;
	}

	void OptimizationContext::recordPassTime(const std::string& passName,
		std::chrono::milliseconds duration) {
		passTimes[passName] = duration;
	}

	size_t OptimizationContext::getTransformationCount(const std::string& passName) const {
		auto it = transformationCounts.find(passName);
		return (it != transformationCounts.end()) ? it->second : 0;
	}

	std::chrono::milliseconds OptimizationContext::getPassTime(const std::string& passName) const {
		auto it = passTimes.find(passName);
		return (it != passTimes.end()) ? it->second : std::chrono::milliseconds(0);
	}

	void OptimizationContext::reset() {
		transformationCounts.clear();
		passTimes.clear();
		usedVariables.clear();
		usedFunctions.clear();
		usedClasses.clear();
		modified = false;
	}

} // namespace optimizer::base
