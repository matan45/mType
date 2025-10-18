#include "OptimizationResult.hpp"
#include <sstream>

namespace optimizer {

	OptimizationResult::OptimizationResult()
		: totalTransformations(0)
		, totalTime(0)
		, optimizationSuccessful(true) {
	}

	void OptimizationResult::addPassMetrics(const PassMetrics& metrics) {
		// Check if we already have metrics for this pass (for fixed-point iteration)
		bool found = false;
		for (auto& existingMetrics : passMetrics) {
			if (existingMetrics.passName == metrics.passName) {
				// Aggregate metrics for the same pass across iterations
				existingMetrics.transformationsApplied += metrics.transformationsApplied;
				existingMetrics.executionTime += metrics.executionTime;
				existingMetrics.modified = existingMetrics.modified || metrics.modified;
				found = true;
				break;
			}
		}

		// If this is the first time we see this pass, add it
		if (!found) {
			passMetrics.push_back(metrics);
		}

		// Update totals
		totalTransformations += metrics.transformationsApplied;
		totalTime += metrics.executionTime;
	}

	void OptimizationResult::addWarning(const std::string& warning) {
		warnings.push_back(warning);
	}

	void OptimizationResult::addError(const std::string& error) {
		errors.push_back(error);
		optimizationSuccessful = false;
	}

	std::string OptimizationResult::generateReport() const {
		std::ostringstream report;

		report << "Optimization Summary:\n";
		report << "  Total Transformations: " << totalTransformations << "\n";
		report << "  Total Time: " << totalTime.count() << " ms\n";
		report << "  Status: " << (optimizationSuccessful ? "Success" : "Failed") << "\n";

		if (!passMetrics.empty()) {
			report << "\nPass Details:\n";
			for (const auto& metrics : passMetrics) {
				report << "  " << metrics.passName << ":\n";
				report << "    Transformations: " << metrics.transformationsApplied << "\n";
				report << "    Time: " << metrics.executionTime.count() << " ms\n";
				report << "    Modified: " << (metrics.modified ? "Yes" : "No") << "\n";
			}
		}

		if (!warnings.empty()) {
			report << "\nWarnings:\n";
			for (const auto& warning : warnings) {
				report << "  - " << warning << "\n";
			}
		}

		if (!errors.empty()) {
			report << "\nErrors:\n";
			for (const auto& error : errors) {
				report << "  - " << error << "\n";
			}
		}

		return report.str();
	}

} // namespace optimizer
