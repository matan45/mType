#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace optimizer {

	/**
	 * Metrics for a single optimization pass execution
	 */
	struct PassMetrics {
		std::string passName;
		size_t transformationsApplied;
		std::chrono::milliseconds executionTime;
		bool modified;

		PassMetrics(const std::string& name = "",
			size_t transformations = 0,
			std::chrono::milliseconds time = std::chrono::milliseconds(0),
			bool wasModified = false)
			: passName(name)
			, transformationsApplied(transformations)
			, executionTime(time)
			, modified(wasModified) {
		}
	};

	/**
	 * Results and metrics from optimization pass execution
	 */
	class OptimizationResult {
	private:
		std::vector<PassMetrics> passMetrics;
		size_t totalTransformations;
		std::chrono::milliseconds totalTime;
		bool optimizationSuccessful;
		std::vector<std::string> warnings;
		std::vector<std::string> errors;

	public:
		OptimizationResult();

		// Add pass result
		void addPassMetrics(const PassMetrics& metrics);

		// Query results
		const std::vector<PassMetrics>& getPassMetrics() const { return passMetrics; }
		size_t getTotalTransformations() const { return totalTransformations; }
		std::chrono::milliseconds getTotalTime() const { return totalTime; }
		bool wasSuccessful() const { return optimizationSuccessful; }

		// Warnings and errors
		void addWarning(const std::string& warning);
		void addError(const std::string& error);
		const std::vector<std::string>& getWarnings() const { return warnings; }
		const std::vector<std::string>& getErrors() const { return errors; }

		void setSuccessful(bool success) { optimizationSuccessful = success; }

		// Reporting
		std::string generateReport() const;
	};

} // namespace optimizer
