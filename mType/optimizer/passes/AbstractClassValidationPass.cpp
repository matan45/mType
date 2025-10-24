#include "AbstractClassValidationPass.hpp"
#include "../OptimizationResult.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../ast/nodes/classes/MethodNode.hpp"
#include "../../ast/nodes/classes/NewNode.hpp"
#include "../../errors/AbstractClassException.hpp"
#include <sstream>
#include <iostream>

namespace optimizer::passes {

	// ========== AbstractValidationTransformer ==========

	AbstractClassValidationPass::AbstractValidationTransformer::AbstractValidationTransformer(
		base::OptimizationContext* ctx, size_t& count, std::vector<std::string>& warns)
		: ASTTransformer(ctx), validationCount(count), warnings(warns)
	{
	}

	bool AbstractClassValidationPass::AbstractValidationTransformer::isAbstractClass(
		const std::string& className) const
	{
		return abstractClasses.find(className) != abstractClasses.end();
	}

	std::unordered_set<std::string> AbstractClassValidationPass::AbstractValidationTransformer::collectAbstractMethods(
		ast::ClassNode* node)
	{
		std::unordered_set<std::string> abstractMethods;

		for (const auto& method : node->getMethods()) {
			if (auto* methodNode = dynamic_cast<ast::MethodNode*>(method.get())) {
				if (methodNode->isAbstract()) {
					abstractMethods.insert(methodNode->getName());
				}
			}
		}

		return abstractMethods;
	}

	void AbstractClassValidationPass::AbstractValidationTransformer::validateNewNode(ast::NewNode* node)
	{
		const std::string& className = node->getClassName();

		// Check if trying to instantiate an abstract class
		if (isAbstractClass(className)) {
			// Note: This is a compile-time optimization check
			// Runtime validation also exists in the evaluator
			std::string warning = "Detected instantiation of abstract class '" + className +
				"' at optimization time. This will fail at runtime.";
			warnings.push_back(warning);
			std::cerr << "Warning: " << warning << std::endl;
			validationCount++;
		}
	}

	void AbstractClassValidationPass::AbstractValidationTransformer::validateClassNode(ast::ClassNode* node)
	{
		const std::string& className = node->getClassName();
		bool isAbstract = node->isAbstract();
		bool isFinal = node->isFinal();

		// Validate: abstract and final are mutually exclusive
		// (Parser should already catch this, but double-check during optimization)
		if (isAbstract && isFinal) {
			std::string warning = "Class '" + className + "' is both abstract and final. This should have been caught by the parser.";
			warnings.push_back(warning);
			std::cerr << "Warning: " << warning << std::endl;
		}

		// Collect abstract methods
		auto abstractMethods = collectAbstractMethods(node);

		// If class is abstract, track it
		if (isAbstract || !abstractMethods.empty()) {
			abstractClasses[className] = abstractMethods;
		}

		// Track parent relationship
		if (node->hasParentClass()) {
			classParents[className] = node->getParentClassName();
		}

		// Validate: concrete class should not have abstract methods
		if (!isAbstract && !abstractMethods.empty()) {
			std::ostringstream oss;
			oss << "Concrete class '" << className << "' has abstract methods: ";
			bool first = true;
			for (const auto& method : abstractMethods) {
				if (!first) oss << ", ";
				oss << method;
				first = false;
			}
			oss << ". Mark the class as abstract or implement the methods.";

			std::string warning = oss.str();
			warnings.push_back(warning);
			std::cerr << "Warning: " << warning << std::endl;
			validationCount++;
		}

		validationCount++;
	}

	std::unique_ptr<ast::ASTNode> AbstractClassValidationPass::AbstractValidationTransformer::visitProgramNode(
		ast::ProgramNode* node)
	{
		// First pass: collect all class information
		for (const auto& statement : node->getStatements()) {
			if (auto* classNode = dynamic_cast<ast::ClassNode*>(statement.get())) {
				validateClassNode(classNode);
			}
		}

		// Second pass: validate NewNodes with collected class information
		return ASTTransformer::visitProgramNode(node);
	}

	std::unique_ptr<ast::ASTNode> AbstractClassValidationPass::AbstractValidationTransformer::visitClassNode(
		ast::ClassNode* node)
	{
		// Class already validated in first pass, just transform children
		return ASTTransformer::visitClassNode(node);
	}

	std::unique_ptr<ast::ASTNode> AbstractClassValidationPass::AbstractValidationTransformer::visitNewNode(
		ast::NewNode* node)
	{
		validateNewNode(node);
		return ASTTransformer::visitNewNode(node);
	}

	// ========== AbstractClassValidationPass ==========

	AbstractClassValidationPass::AbstractClassValidationPass()
		: OptimizationPass("Abstract Class Validation", base::PassType::VALIDATION), validationsPassed(0)
	{
	}

	std::unique_ptr<ast::ASTNode> AbstractClassValidationPass::optimize(
		std::unique_ptr<ast::ASTNode> node,
		base::OptimizationContext& context)
	{
		AbstractValidationTransformer transformer(&context, validationsPassed, validationWarnings);

		// Manual dispatch - check node type and call appropriate visit method
		std::unique_ptr<ast::ASTNode> result;
		if (auto* programNode = dynamic_cast<ast::ProgramNode*>(node.get())) {
			result = transformer.visitProgramNode(programNode);
		}

		// If transformation occurred, return transformed node
		// Otherwise, return original node
		if (result) {
			if (transformer.wasModified()) {
				context.setModified(true);
			}
			return result;
		}

		return node;
	}

	std::string AbstractClassValidationPass::getDescription() const
	{
		return "Validates abstract class usage and constraints";
	}

	void AbstractClassValidationPass::reportMetrics(OptimizationResult& result) const
	{
		// Add warnings to result
		for (const auto& warning : validationWarnings) {
			result.addWarning(warning);
		}

		// Add pass metrics
		PassMetrics metrics(getName(), validationsPassed, getExecutionTime(), false);
		result.addPassMetrics(metrics);
	}

	void AbstractClassValidationPass::reset()
	{
		validationsPassed = 0;
		validationWarnings.clear();
	}

} // namespace optimizer::passes
