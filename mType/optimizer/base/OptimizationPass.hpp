#pragma once

#include <memory>
#include <cstddef>
#include <string>
#include <set>
#include <chrono>
#include "../../ast/ASTNode.hpp"

namespace optimizer {
	namespace base {
		class OptimizationContext;
	}

	class OptimizationResult;
}

namespace optimizer::base {

	// Types of optimization passes
	enum class PassType {
		ANALYSIS,        // Collects information without modification
		TRANSFORMATION,  // Modifies the AST
		VALIDATION      // Verifies correctness
	};

	// Dependencies that passes may require
	enum class PassDependency {
		REQUIRES_CONSTANT_FOLDING,  // Needs constant expressions evaluated
		REQUIRES_TYPE_INFO,          // Needs type information
		INDEPENDENT                  // Runs independently
	};

	/**
	 * Abstract base class for all optimization passes.
	 * Follows Single Responsibility Principle - each pass handles one optimization.
	 * Follows Open/Closed Principle - new passes extend without modifying existing code.
	 */
	class OptimizationPass {
	protected:
		std::string passName;
		PassType passType;
		std::set<PassDependency> dependencies;
		bool enabled;

		// Metrics
		size_t transformationCount;
		std::chrono::milliseconds executionTime;

	public:
		explicit OptimizationPass(const std::string& name, PassType type);
		virtual ~OptimizationPass() = default;

		// Main optimization entry point
		virtual std::unique_ptr<ast::ASTNode> optimize(
			std::unique_ptr<ast::ASTNode> node,
			OptimizationContext& context
		) = 0;

		// Pass metadata
		virtual std::string getName() const { return passName; }
		virtual std::string getDescription() const = 0;
		virtual PassType getType() const { return passType; }
		virtual std::set<PassDependency> getDependencies() const { return dependencies; }

		// Configuration
		virtual void setEnabled(bool enable) { enabled = enable; }
		virtual bool isEnabled() const { return enabled; }

		// Metrics
		size_t getTransformationCount() const { return transformationCount; }
		std::chrono::milliseconds getExecutionTime() const { return executionTime; }

		// Metrics and reporting
		virtual void reportMetrics(OptimizationResult& result) const;
		virtual void reset();

	protected:
		void recordTransformation() { ++transformationCount; }
		void setExecutionTime(std::chrono::milliseconds time) { executionTime = time; }
	};

} // namespace optimizer::base
