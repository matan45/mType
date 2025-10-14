#include "Optimizer.hpp"
#include "passes/DeadCodeEliminationPass.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../ast/nodes/statements/IfNode.hpp"
#include "../ast/nodes/statements/WhileNode.hpp"
#include "../ast/nodes/statements/DoWhileNode.hpp"
#include "../ast/nodes/statements/ForNode.hpp"
#include "../ast/nodes/statements/ForEachNode.hpp"
#include "../ast/nodes/statements/SwitchNode.hpp"
#include "../ast/nodes/statements/TryNode.hpp"
#include "../ast/nodes/functions/FunctionNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../ast/nodes/classes/ConstructorNode.hpp"

namespace optimizer {

	using namespace ast::nodes::statements;
	using namespace ast::nodes::functions;
	using namespace ast::nodes::classes;

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

	// Forward declare the count function from DeadCodeEliminationPass.cpp
	namespace passes {
		extern size_t countNodes(const ast::ASTNode* node);
	}

	size_t Optimizer::countASTNodes(const ast::ASTNode* node) const {
		return passes::countNodes(node);
	}

} // namespace optimizer
