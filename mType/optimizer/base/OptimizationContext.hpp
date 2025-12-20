#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include "../../environment/Environment.hpp"
#include "../OptimizationConfig.hpp"

namespace optimizer::base {

	/**
	 * Shared state and services for optimization passes.
	 * Provides context information and tracking during optimization.
	 * Follows Dependency Inversion Principle - depends on abstractions.
	 */
	class OptimizationContext {
	private:
		std::shared_ptr<environment::Environment> environment;
		const OptimizationConfig& config;

		// Tracking and metrics
		std::unordered_map<std::string, size_t> transformationCounts;
		std::unordered_map<std::string, std::chrono::milliseconds> passTimes;

		// Symbol information for optimization
		std::unordered_set<std::string> usedVariables;
		std::unordered_set<std::string> usedFunctions;
		std::unordered_set<std::string> usedClasses;

		// Optimization state
		bool modified;

	public:
		explicit OptimizationContext(
			std::shared_ptr<environment::Environment> env,
			const OptimizationConfig& cfg
		);

		// Configuration access
		const OptimizationConfig& getConfig() const { return config; }

		// Environment access
		std::shared_ptr<environment::Environment> getEnvironment() const { return environment; }

		// Symbol tracking
		void markVariableUsed(const std::string& name);
		void markFunctionUsed(const std::string& name);
		void markClassUsed(const std::string& name);
		bool isVariableUsed(const std::string& name) const;
		bool isFunctionUsed(const std::string& name) const;
		bool isClassUsed(const std::string& name) const;

		// Metrics
		void recordTransformation(const std::string& passName);
		void recordPassTime(const std::string& passName,
			std::chrono::milliseconds duration);

		size_t getTransformationCount(const std::string& passName) const;
		std::chrono::milliseconds getPassTime(const std::string& passName) const;

		// Modification tracking
		void setModified(bool wasModified) { modified = wasModified; }
		bool wasModified() const { return modified; }

		// Reset for new optimization run
		void reset();
	};

} // namespace optimizer::base
