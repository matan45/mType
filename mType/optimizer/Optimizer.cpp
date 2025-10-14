#include "Optimizer.hpp"

namespace optimizer {

	Optimizer::Optimizer(const OptimizationConfig& cfg)
		: config(cfg)
		, passManager(std::make_unique<OptimizationPassManager>(cfg)) {

		// Register default optimization passes
		passManager->registerDefaultPasses();
	}

	std::unique_ptr<ast::ASTNode> Optimizer::optimize(
		std::unique_ptr<ast::ASTNode> ast,
		std::shared_ptr<environment::Environment> environment) {

		// Create optimization context
		base::OptimizationContext context(environment, config);

		// Run optimization passes
		ast = passManager->runPasses(std::move(ast), context);

		return ast;
	}

	void Optimizer::setConfig(const OptimizationConfig& cfg) {
		config = cfg;
		passManager->setConfig(cfg);
	}

	void Optimizer::enablePass(const std::string& passName) {
		passManager->enablePass(passName);
	}

	void Optimizer::disablePass(const std::string& passName) {
		passManager->disablePass(passName);
	}

	OptimizationResult Optimizer::getLastResult() const {
		return passManager->getLastResult();
	}

} // namespace optimizer
