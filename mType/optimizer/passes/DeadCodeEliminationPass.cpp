/**
 * Dead Code Elimination Pass
 *
 * CURRENT STATUS: FULLY FUNCTIONAL (Phase 2 Complete)
 * ===================================================
 *
 * This pass removes unreachable code after control flow terminating statements.
 *
 * What it does:
 * - Traverses the AST and identifies unreachable code after return/break/continue/throw
 * - Clones nodes and removes dead statements
 * - Returns transformed AST with dead code removed
 * - Reports metrics (statements removed, execution time)
 *
 * Implementation:
 * - Uses AST node cloning infrastructure
 * - Creates new transformed nodes with dead code truncated
 * - Recursively processes nested blocks and control flow structures
 */

#include "DeadCodeEliminationPass.hpp"
#include "../OptimizationResult.hpp"
#include "../../parser/TypeParser.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/BlockNode.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../ast/nodes/statements/IfNode.hpp"
#include "../../ast/nodes/statements/WhileNode.hpp"
#include "../../ast/nodes/statements/DoWhileNode.hpp"
#include "../../ast/nodes/statements/ForNode.hpp"
#include "../../ast/nodes/statements/ForEachNode.hpp"
#include "../../ast/nodes/statements/SwitchNode.hpp"
#include "../../ast/nodes/statements/CaseNode.hpp"
#include "../../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../../ast/nodes/statements/TryNode.hpp"
#include "../../ast/nodes/statements/CatchNode.hpp"
#include "../../ast/nodes/statements/BreakNode.hpp"
#include "../../ast/nodes/expressions/LambdaNode.hpp"
#include "../../ast/nodes/statements/ContinueNode.hpp"
#include "../../ast/nodes/statements/ThrowNode.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../ast/nodes/functions/ReturnNode.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../ast/nodes/classes/MethodNode.hpp"
#include "../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../ast/nodes/classes/SuperConstructorCallNode.hpp"
#include <chrono>
#include <iostream>

namespace optimizer::passes {

	using namespace ast;
	using namespace ast::nodes::statements;
	using namespace ast::nodes::functions;
	using namespace ast::nodes::classes;

	// Debug flag - set to true to enable debug logging
	constexpr bool DCE_DEBUG = false;

	// ================= Node Counting Utility =================

	// Recursively count all nodes in the AST
	size_t countNodes(const ast::ASTNode* node) {
		if (!node) {
			return 0;
		}

		size_t count = 1; // Count this node

		// Count children based on node type
		if (auto* programNode = dynamic_cast<const ProgramNode*>(node)) {
			for (const auto& stmt : programNode->getStatements()) {
				count += countNodes(stmt.get());
			}
		}
		else if (auto* blockNode = dynamic_cast<const BlockNode*>(node)) {
			for (const auto& stmt : blockNode->getStatements()) {
				count += countNodes(stmt.get());
			}
		}
		else if (auto* functionNode = dynamic_cast<const FunctionNode*>(node)) {
			count += countNodes(functionNode->getBodyPtr());
		}
		else if (auto* methodNode = dynamic_cast<const MethodNode*>(node)) {
			count += countNodes(methodNode->getBodyPtr());
		}
		else if (auto* constructorNode = dynamic_cast<const ConstructorNode*>(node)) {
			count += countNodes(constructorNode->getBodyPtr());
		}
		else if (auto* ifNode = dynamic_cast<const IfNode*>(node)) {
			count += countNodes(ifNode->getCondition());
			count += countNodes(ifNode->getThenStatement());
			if (ifNode->hasElseStatement()) {
				count += countNodes(ifNode->getElseStatement());
			}
		}
		else if (auto* whileNode = dynamic_cast<const WhileNode*>(node)) {
			count += countNodes(whileNode->getCondition());
			count += countNodes(whileNode->getBody());
		}
		else if (auto* doWhileNode = dynamic_cast<const DoWhileNode*>(node)) {
			count += countNodes(doWhileNode->getBody());
			count += countNodes(doWhileNode->getCondition());
		}
		else if (auto* forNode = dynamic_cast<const ForNode*>(node)) {
			count += countNodes(forNode->getInitialization());
			count += countNodes(forNode->getCondition());
			count += countNodes(forNode->getUpdate());
			count += countNodes(forNode->getBody());
		}
		else if (auto* forEachNode = dynamic_cast<const ForEachNode*>(node)) {
			count += countNodes(forEachNode->getCollection());
			count += countNodes(forEachNode->getBody());
		}
		else if (auto* switchNode = dynamic_cast<const SwitchNode*>(node)) {
			count += countNodes(switchNode->getExpression());
			for (const auto& caseNode : switchNode->getCases()) {
				count += countNodes(caseNode.get());
			}
		}
		else if (auto* caseNode = dynamic_cast<const CaseNode*>(node)) {
			count += countNodes(caseNode->getValue());
			for (const auto& stmt : caseNode->getStatements()) {
				count += countNodes(stmt.get());
			}
		}
		else if (auto* defaultCaseNode = dynamic_cast<const DefaultCaseNode*>(node)) {
			for (const auto& stmt : defaultCaseNode->getStatements()) {
				count += countNodes(stmt.get());
			}
		}
		else if (auto* tryNode = dynamic_cast<const TryNode*>(node)) {
			count += countNodes(tryNode->getTryBlock());
			for (const auto& catchBlock : tryNode->getCatchBlocks()) {
				count += countNodes(catchBlock.get());
			}
			if (tryNode->hasFinallyBlock()) {
				count += countNodes(tryNode->getFinallyBlock());
			}
		}
		else if (auto* catchNode = dynamic_cast<const CatchNode*>(node)) {
			count += countNodes(catchNode->getBody());
		}
		else if (auto* lambdaNode = dynamic_cast<const ast::nodes::expressions::LambdaNode*>(node)) {
			count += countNodes(lambdaNode->getBody());
		}

		return count;
	}

	// ================= DCETransformer Implementation =================

	DeadCodeEliminationPass::DCETransformer::DCETransformer(
		base::OptimizationContext* ctx,
		size_t& count)
		: ASTTransformer(ctx)
		, removedCount(count) {
	}

	bool DeadCodeEliminationPass::DCETransformer::isTerminatingStatement(ast::ASTNode* node) const {
		if (!node) {
			return false;
		}

		// Check if the node is a terminating statement
		return dynamic_cast<ReturnNode*>(node) != nullptr ||
			dynamic_cast<BreakNode*>(node) != nullptr ||
			dynamic_cast<ContinueNode*>(node) != nullptr ||
			dynamic_cast<ThrowNode*>(node) != nullptr;
	}

	// Transform statements by removing dead code after terminating statements
	// Returns a new vector with dead code removed, or empty vector if no changes
	std::vector<std::unique_ptr<ast::ASTNode>> DeadCodeEliminationPass::DCETransformer::transformStatements(
		const std::vector<std::unique_ptr<ast::ASTNode>>& statements) {

		if (DCE_DEBUG) {
			std::cout << "[DCE] transformStatements: Analyzing " << statements.size() << " statements\n";
		}

		std::vector<std::unique_ptr<ast::ASTNode>> transformedStatements;
		bool foundTerminator = false;
		size_t terminatorIndex = SIZE_MAX;

		// Find the first terminating statement
		for (size_t i = 0; i < statements.size(); ++i) {
			if (isTerminatingStatement(statements[i].get())) {
				if (DCE_DEBUG) {
					std::cout << "[DCE] transformStatements: Found terminator at index " << i
					          << " (type: " << typeid(*statements[i]).name() << ")\n";
				}
				foundTerminator = true;
				terminatorIndex = i;
				break;
			}
		}

		// If we found a terminator and there are statements after it, we have dead code
		if (foundTerminator && terminatorIndex + 1 < statements.size()) {
			size_t deadCount = statements.size() - (terminatorIndex + 1);
			if (DCE_DEBUG) {
				std::cout << "[DCE] transformStatements: Removing " << deadCount << " dead statements after terminator\n";
			}
			removedCount += deadCount;
			modified = true;
			context->recordTransformation("DeadCodeElimination");

			// Transform all statements up to and including the terminator
			transformedStatements.reserve(terminatorIndex + 1);
			for (size_t i = 0; i <= terminatorIndex; ++i) {
				if (statements[i]) {
					// Recursively transform child nodes (e.g., function bodies)
					auto transformed = transformChild(statements[i].get());
					if (transformed) {
						transformedStatements.push_back(std::move(transformed));
					} else {
						transformedStatements.push_back(statements[i]->clone());
					}
				}
			}
		} else {
			if (DCE_DEBUG) {
				std::cout << "[DCE] transformStatements: No dead code found, transforming all statements recursively\n";
			}
			// No dead code at this level - but still need to recursively transform children
			transformedStatements.reserve(statements.size());
			for (const auto& stmt : statements) {
				if (stmt) {
					// Recursively transform child nodes (e.g., function bodies)
					auto transformed = transformChild(stmt.get());
					if (transformed) {
						transformedStatements.push_back(std::move(transformed));
					} else {
						transformedStatements.push_back(stmt->clone());
					}
				}
			}
		}

		return transformedStatements;
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitProgramNode(ProgramNode* node) {
		if (DCE_DEBUG) {
			std::cout << "[DCE] visitProgramNode: Processing " << node->getStatements().size() << " statements\n";
		}

		// Transform statements - remove dead code
		auto transformedStatements = transformStatements(node->getStatements());

		if (DCE_DEBUG) {
			std::cout << "[DCE] visitProgramNode: After transform, have " << transformedStatements.size() << " statements\n";
		}

		// Create new ProgramNode with transformed statements
		return std::make_unique<ProgramNode>(std::move(transformedStatements), node->getLocation());
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitBlockNode(BlockNode* node) {
		if (DCE_DEBUG) {
			std::cout << "[DCE] visitBlockNode: Processing " << node->getStatements().size() << " statements\n";
		}

		// Transform statements - remove dead code
		auto transformedStatements = transformStatements(node->getStatements());

		if (DCE_DEBUG) {
			std::cout << "[DCE] visitBlockNode: After transform, have " << transformedStatements.size() << " statements\n";
		}

		// Create new BlockNode with transformed statements
		return std::make_unique<BlockNode>(std::move(transformedStatements), node->getLocation());
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitFunctionNode(FunctionNode* node) {
		if (DCE_DEBUG) {
			std::cout << "[DCE] visitFunctionNode: Processing function '" << node->getName() << "'\n";
		}

		// Transform the function body (which is a BlockNode)
		auto body = node->getBodyPtr();
		if (!body) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] visitFunctionNode: No body, using clone()\n";
			}
			return nullptr; // Use clone() for functions without body
		}

		// Transform the body through the visitor - this will trigger visitBlockNode
		auto transformedBodyUnique = transformChild(body);

		// If transformation occurred, create new FunctionNode with transformed body
		if (transformedBodyUnique) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] visitFunctionNode: Body was transformed, creating new FunctionNode\n";
			}

			// Convert unique_ptr to shared_ptr for FunctionNode constructor
			std::shared_ptr<ast::ASTNode> transformedBody = std::move(transformedBodyUnique);

			// Create new FunctionNode with transformed body
			auto transformedFunction = std::make_unique<FunctionNode>(
				node->getName(),
				node->getGenericReturnType(),
				node->getGenericParameters(),
				transformedBody,
				node->getGenericTypeParameters(),
				node->getIsAsync(),
				node->getLocation()
			);

			// Preserve visibility modifier
			transformedFunction->setVisibility(node->getVisibility());

			return transformedFunction;
		}

		if (DCE_DEBUG) {
			std::cout << "[DCE] visitFunctionNode: No transformation, using clone()\n";
		}

		// No transformation - use clone()
		return nullptr;
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitMethodNode(MethodNode* node) {
		// Transform the method body (which is a BlockNode)
		auto body = node->getBodyPtr();
		if (!body) {
			return nullptr; // Use clone() for methods without body
		}

		// Transform the body through the visitor - this will trigger visitBlockNode
		auto transformedBodyUnique = transformChild(body);

		// If transformation occurred, create new MethodNode with transformed body
		if (transformedBodyUnique) {
			// Convert unique_ptr to shared_ptr for MethodNode constructor
			std::shared_ptr<ast::ASTNode> transformedBody = std::move(transformedBodyUnique);

			// Create new MethodNode with transformed body
			auto transformedMethod = std::make_unique<MethodNode>(
				node->getName(),
				node->getGenericReturnType(),
				node->getGenericParameters(),
				transformedBody,
				node->getIsStatic(),
				node->getGenericTypeParameters(),
				node->getAccessModifier(),
				node->getIsAsync(),
				node->getLocation()
			);

			return transformedMethod;
		}

		// No transformation - use clone()
		return nullptr;
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitConstructorNode(ConstructorNode* node) {
		// Transform the constructor body (which is a BlockNode)
		auto body = node->getBodyPtr();
		if (!body) {
			return nullptr; // Use clone() for constructors without body
		}

		// Transform the body through the visitor - this will trigger visitBlockNode
		auto transformedBodyUnique = transformChild(body);

		// If transformation occurred, create new ConstructorNode with transformed body
		if (transformedBodyUnique) {
			// Convert unique_ptr to shared_ptr for ConstructorNode constructor
			std::shared_ptr<ast::ASTNode> transformedBody = std::move(transformedBodyUnique);

			// Create new ConstructorNode with transformed body
			auto transformedConstructor = std::make_unique<ConstructorNode>(
				node->getParametersWithTypes(),
				transformedBody,
				node->getAccessModifier(),
				node->getLocation()
			);

			// Clone super initializer if present (cannot transform, so we clone it)
			if (node->hasSuperInitializer()) {
				auto clonedSuper = std::unique_ptr<ast::nodes::classes::SuperConstructorCallNode>(
					static_cast<ast::nodes::classes::SuperConstructorCallNode*>(
						node->getSuperInitializer()->clone().release()
					)
				);
				transformedConstructor->setSuperInitializer(std::move(clonedSuper));
			}

			return transformedConstructor;
		}

		// No transformation - use clone()
		return nullptr;
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitIfNode(IfNode* node) {
		if (DCE_DEBUG) {
			std::cout << "[DCE] visitIfNode: Processing if statement\n";
		}

		// Transform condition (no dead code elimination here, but recursively check)
		auto transformedCondition = transformChild(node->getCondition());
		auto* conditionToUse = transformedCondition ? transformedCondition.get() : node->getCondition();

		// Transform then branch
		auto transformedThen = transformChild(node->getThenStatement());
		auto* thenToUse = transformedThen ? transformedThen.get() : node->getThenStatement();

		// Transform else branch if present
		std::unique_ptr<ast::ASTNode> transformedElse = nullptr;
		ASTNode* elseToUse = nullptr;
		if (node->hasElseStatement() && node->getElseStatement()) {
			transformedElse = transformChild(node->getElseStatement());
			elseToUse = transformedElse ? transformedElse.get() : node->getElseStatement();
		}

		// If any branch was transformed, create new IfNode
		if (transformedCondition || transformedThen || transformedElse) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] visitIfNode: Creating new IfNode with transformed branches\n";
			}

			auto newIfNode = std::make_unique<IfNode>(
				transformedCondition ? std::move(transformedCondition) : conditionToUse->clone(),
				transformedThen ? std::move(transformedThen) : thenToUse->clone(),
				elseToUse ? (transformedElse ? std::move(transformedElse) : elseToUse->clone()) : nullptr,
				node->getLocation()
			);

			return newIfNode;
		}

		if (DCE_DEBUG) {
			std::cout << "[DCE] visitIfNode: No transformation, using clone()\n";
		}

		// No transformation - use clone()
		return nullptr;
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitWhileNode(WhileNode* node) {
		// Transform condition and body
		auto transformedCondition = transformChild(node->getCondition());
		auto transformedBody = transformChild(node->getBody());

		// If either was transformed, create new WhileNode
		if (transformedCondition || transformedBody) {
			return std::make_unique<WhileNode>(
				transformedCondition ? std::move(transformedCondition) : node->getCondition()->clone(),
				transformedBody ? std::move(transformedBody) : node->getBody()->clone(),
				node->getLocation()
			);
		}

		return nullptr; // Use clone()
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitDoWhileNode(DoWhileNode* node) {
		// Transform body and condition
		auto transformedBody = transformChild(node->getBody());
		auto transformedCondition = transformChild(node->getCondition());

		// If either was transformed, create new DoWhileNode
		if (transformedBody || transformedCondition) {
			return std::make_unique<DoWhileNode>(
				transformedBody ? std::move(transformedBody) : node->getBody()->clone(),
				transformedCondition ? std::move(transformedCondition) : node->getCondition()->clone(),
				node->getLocation()
			);
		}

		return nullptr; // Use clone()
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitForNode(ForNode* node) {
		// Transform all parts of for loop
		auto transformedInit = node->getInitialization() ? transformChild(node->getInitialization()) : nullptr;
		auto transformedCondition = node->getCondition() ? transformChild(node->getCondition()) : nullptr;
		auto transformedUpdate = node->getUpdate() ? transformChild(node->getUpdate()) : nullptr;
		auto transformedBody = transformChild(node->getBody());

		// If any part was transformed, create new ForNode
		if (transformedInit || transformedCondition || transformedUpdate || transformedBody) {
			return std::make_unique<ForNode>(
				transformedInit ? std::move(transformedInit) : (node->getInitialization() ? node->getInitialization()->clone() : nullptr),
				transformedCondition ? std::move(transformedCondition) : (node->getCondition() ? node->getCondition()->clone() : nullptr),
				transformedUpdate ? std::move(transformedUpdate) : (node->getUpdate() ? node->getUpdate()->clone() : nullptr),
				transformedBody ? std::move(transformedBody) : node->getBody()->clone(),
				node->getLocation()
			);
		}

		return nullptr; // Use clone()
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitForEachNode(ForEachNode* node) {
		// Transform collection and body
		auto transformedCollection = transformChild(node->getCollection());
		auto transformedBody = transformChild(node->getBody());

		// If either was transformed, create new ForEachNode
		if (transformedCollection || transformedBody) {
			return std::make_unique<ForEachNode>(
				node->getVariableName(),
				node->getVariableTypeInfo(),
				transformedCollection ? std::move(transformedCollection) : node->getCollection()->clone(),
				transformedBody ? std::move(transformedBody) : node->getBody()->clone(),
				node->getLocation()
			);
		}

		return nullptr; // Use clone()
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitSwitchNode(SwitchNode* node) {
		// Transform expression
		auto transformedExpr = transformChild(node->getExpression());

		// Transform each case
		bool anyCaseTransformed = false;
		std::vector<std::unique_ptr<ast::ASTNode>> transformedCases;
		transformedCases.reserve(node->getCases().size());

		for (const auto& caseNode : node->getCases()) {
			auto transformed = transformChild(caseNode.get());
			if (transformed) {
				anyCaseTransformed = true;
				transformedCases.push_back(std::move(transformed));
			} else {
				transformedCases.push_back(caseNode->clone());
			}
		}

		// If expression or any case was transformed, create new SwitchNode
		if (transformedExpr || anyCaseTransformed) {
			auto newSwitch = std::make_unique<SwitchNode>(
				transformedExpr ? std::move(transformedExpr) : node->getExpression()->clone(),
				node->getLocation()
			);

			for (auto& caseNode : transformedCases) {
				newSwitch->addCase(std::move(caseNode));
			}

			return newSwitch;
		}

		return nullptr; // Use clone()
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitCaseNode(CaseNode* node) {
		if (DCE_DEBUG) {
			std::cout << "[DCE] visitCaseNode: Processing case with " << node->getStatements().size() << " statements\n";
		}

		// Transform the case value expression
		auto transformedValue = transformChild(node->getValue());

		// Transform statements - this is where we remove dead code after return/break
		size_t originalSize = node->getStatements().size();
		auto transformedStatements = transformStatements(node->getStatements());

		// Check if anything was transformed (value changed OR statement count changed)
		bool hasChanges = transformedValue != nullptr || transformedStatements.size() != originalSize;

		if (hasChanges) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] visitCaseNode: Creating new CaseNode with " << transformedStatements.size()
				          << " statements (was " << originalSize << ")\n";
			}

			// Create new CaseNode with transformed value and statements
			auto newCase = std::make_unique<CaseNode>(
				transformedValue ? std::move(transformedValue) : node->getValue()->clone(),
				node->getLocation()
			);

			// Add transformed statements
			for (auto& stmt : transformedStatements) {
				newCase->addStatement(std::move(stmt));
			}

			return newCase;
		}

		if (DCE_DEBUG) {
			std::cout << "[DCE] visitCaseNode: No transformation, using clone()\n";
		}

		return nullptr; // Use clone()
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitDefaultCaseNode(DefaultCaseNode* node) {
		if (DCE_DEBUG) {
			std::cout << "[DCE] visitDefaultCaseNode: Processing default case with " << node->getStatements().size() << " statements\n";
		}

		// Transform statements - this is where we remove dead code after return/break
		size_t originalSize = node->getStatements().size();
		auto transformedStatements = transformStatements(node->getStatements());

		// Check if anything was transformed (statement count changed)
		if (transformedStatements.size() != originalSize) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] visitDefaultCaseNode: Creating new DefaultCaseNode with " << transformedStatements.size()
				          << " statements (was " << originalSize << ")\n";
			}

			// Create new DefaultCaseNode with transformed statements
			auto newDefaultCase = std::make_unique<DefaultCaseNode>(node->getLocation());

			// Add transformed statements
			for (auto& stmt : transformedStatements) {
				newDefaultCase->addStatement(std::move(stmt));
			}

			return newDefaultCase;
		}

		if (DCE_DEBUG) {
			std::cout << "[DCE] visitDefaultCaseNode: No transformation, using clone()\n";
		}

		return nullptr; // Use clone()
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitTryNode(TryNode* node) {
		// Transform try block
		auto transformedTry = transformChild(node->getTryBlock());

		// Transform catch blocks
		bool anyCatchTransformed = false;
		std::vector<std::unique_ptr<CatchNode>> transformedCatches;
		transformedCatches.reserve(node->getCatchBlocks().size());

		for (const auto& catchBlock : node->getCatchBlocks()) {
			auto transformed = transformChild(catchBlock.get());
			if (transformed) {
				anyCatchTransformed = true;
				transformedCatches.push_back(std::unique_ptr<CatchNode>(
					static_cast<CatchNode*>(transformed.release())
				));
			} else {
				transformedCatches.push_back(std::unique_ptr<CatchNode>(
					static_cast<CatchNode*>(catchBlock->clone().release())
				));
			}
		}

		// Transform finally block
		std::unique_ptr<ast::ASTNode> transformedFinally = nullptr;
		if (node->hasFinallyBlock()) {
			transformedFinally = transformChild(node->getFinallyBlock());
		}

		// If any part was transformed, create new TryNode
		if (transformedTry || anyCatchTransformed || transformedFinally) {
			return std::make_unique<TryNode>(
				transformedTry ? std::move(transformedTry) : node->getTryBlock()->clone(),
				std::move(transformedCatches),
				transformedFinally ? std::move(transformedFinally) :
					(node->hasFinallyBlock() ? node->getFinallyBlock()->clone() : nullptr),
				node->getLocation()
			);
		}

		return nullptr; // Use clone()
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitCatchNode(CatchNode* node) {
		if (DCE_DEBUG) {
			std::cout << "[DCE] visitCatchNode: Processing catch block for exception type '" << node->getExceptionType() << "'\n";
		}

		// Transform the catch body (which should be a BlockNode)
		auto body = node->getBody();
		if (!body) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] visitCatchNode: No body, using clone()\n";
			}
			return nullptr; // Use clone() for catch blocks without body
		}

		// Transform the body through the visitor - this will trigger visitBlockNode
		auto transformedBody = transformChild(body);

		// If transformation occurred, create new CatchNode with transformed body
		if (transformedBody) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] visitCatchNode: Body was transformed, creating new CatchNode\n";
			}

			// Create new CatchNode with transformed body
			auto newCatch = std::make_unique<CatchNode>(
				node->getExceptionType(),
				node->getVariableName(),
				std::move(transformedBody),
				node->getLocation()
			);

			return newCatch;
		}

		if (DCE_DEBUG) {
			std::cout << "[DCE] visitCatchNode: No transformation, using clone()\n";
		}

		// No transformation - use clone()
		return nullptr;
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitLambdaNode(ast::nodes::expressions::LambdaNode* node) {
		if (DCE_DEBUG) {
			std::cout << "[DCE] visitLambdaNode: Processing lambda with " << node->getParameters().size() << " parameters\n";
		}

		// Only transform block lambdas, not expression lambdas
		if (!node->isBlockLambda()) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] visitLambdaNode: Expression lambda, no dead code elimination needed\n";
			}
			return nullptr; // Expression lambdas don't have dead code
		}

		// Transform the lambda body (which should be a BlockNode for block lambdas)
		auto body = node->getBody();
		if (!body) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] visitLambdaNode: No body, using clone()\n";
			}
			return nullptr; // Use clone() for lambdas without body
		}

		// Transform the body through the visitor - this will trigger visitBlockNode
		auto transformedBody = transformChild(body);

		// If transformation occurred, create new LambdaNode with transformed body
		if (transformedBody) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] visitLambdaNode: Body was transformed, creating new LambdaNode\n";
			}

			// Create new LambdaNode with transformed body
			auto newLambda = std::make_unique<ast::nodes::expressions::LambdaNode>(
				node->getParameters(),
				std::move(transformedBody),
				node->getLocation(),
				node->getBodyType(),
				node->getIsAsync()
			);

			return newLambda;
		}

		if (DCE_DEBUG) {
			std::cout << "[DCE] visitLambdaNode: No transformation, using clone()\n";
		}

		// No transformation - use clone()
		return nullptr;
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitClassNode(ast::nodes::classes::ClassNode* node) {
		if (DCE_DEBUG) {
			std::cout << "[DCE] visitClassNode: Processing class '" << node->getClassName() << "'\n";
		}

		// Transform constructors
		bool anyConstructorTransformed = false;
		std::vector<std::unique_ptr<ast::ASTNode>> transformedConstructors;
		transformedConstructors.reserve(node->getConstructors().size());

		for (const auto& constructor : node->getConstructors()) {
			auto transformed = transformChild(constructor.get());
			if (transformed) {
				anyConstructorTransformed = true;
				transformedConstructors.push_back(std::move(transformed));
			} else {
				transformedConstructors.push_back(constructor->clone());
			}
		}

		// Transform methods
		bool anyMethodTransformed = false;
		std::vector<std::unique_ptr<ast::ASTNode>> transformedMethods;
		transformedMethods.reserve(node->getMethods().size());

		for (const auto& method : node->getMethods()) {
			auto transformed = transformChild(method.get());
			if (transformed) {
				anyMethodTransformed = true;
				transformedMethods.push_back(std::move(transformed));
			} else {
				transformedMethods.push_back(method->clone());
			}
		}

		// If any constructor or method was transformed, create new ClassNode
		if (anyConstructorTransformed || anyMethodTransformed) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] visitClassNode: Creating new ClassNode with transformed methods/constructors\n";
			}

			// Create new ClassNode with inheritance constructor to preserve extends/implements
			auto newClass = std::make_unique<ast::nodes::classes::ClassNode>(
				node->getClassName(),
				node->getGenericParameters(),
				node->getParentClassName(),
				node->getImplementedInterfaces(),
				node->getLocation()
			);

			// Clone fields (fields don't have dead code to eliminate)
			for (const auto& field : node->getFields()) {
				if (field) {
					newClass->addField(field->clone());
				}
			}

			// Add transformed constructors
			for (auto& constructor : transformedConstructors) {
				newClass->addConstructor(std::move(constructor));
			}

			// Add transformed methods
			for (auto& method : transformedMethods) {
				newClass->addMethod(std::move(method));
			}

			// Preserve other attributes
			newClass->setFinal(node->isFinal());
			newClass->setVisibility(node->getVisibility());

			return newClass;
		}

		if (DCE_DEBUG) {
			std::cout << "[DCE] visitClassNode: No transformation, using clone()\n";
		}

		// No transformation - use clone()
		return nullptr;
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitAssignmentNode(ast::nodes::statements::AssignmentNode* node) {
		if (DCE_DEBUG) {
			std::cout << "[DCE] visitAssignmentNode: Processing assignment to '" << node->getVariableName() << "'\n";
		}

		// Transform the value expression (which may contain a lambda with dead code)
		auto value = node->getValue();
		if (!value) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] visitAssignmentNode: No value, using clone()\n";
			}
			return nullptr; // Use clone() for assignments without value
		}

		// Transform the value through the visitor - this will trigger visitLambdaNode if it's a lambda
		auto transformedValue = transformChild(value);

		// If transformation occurred, create new AssignmentNode with transformed value
		if (transformedValue) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] visitAssignmentNode: Value was transformed, creating new AssignmentNode\n";
			}

			// Create new AssignmentNode with transformed value
			auto newAssignment = std::make_unique<ast::nodes::statements::AssignmentNode>(
				node->getVariableName(),
				std::move(transformedValue),
				node->getVariableType(),
				node->getClassName(),
				node->getIsFinal(),
				node->getIsStatic(),
				node->getLocation()
			);

			// Preserve visibility modifier
			newAssignment->setVisibility(node->getVisibility());

			return newAssignment;
		}

		if (DCE_DEBUG) {
			std::cout << "[DCE] visitAssignmentNode: No transformation, using clone()\n";
		}

		// No transformation - use clone()
		return nullptr;
	}

	// ================= DeadCodeEliminationPass Implementation =================

	DeadCodeEliminationPass::DeadCodeEliminationPass()
		: OptimizationPass("DeadCodeElimination", base::PassType::TRANSFORMATION)
		, removedStatements(0) {
	}

	std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::optimize(
		std::unique_ptr<ast::ASTNode> node,
		base::OptimizationContext& context) {

		if (DCE_DEBUG) {
			std::cout << "[DCE] ===== Starting Dead Code Elimination Pass =====\n";
			std::cout << "[DCE] Input node type: " << (node ? typeid(*node).name() : "nullptr") << "\n";
		}

		auto startTime = std::chrono::high_resolution_clock::now();

		// Create transformer and run it
		DCETransformer transformer(&context, removedStatements);

		// Manual dispatch - check node type and call appropriate visit method
		std::unique_ptr<ast::ASTNode> result;
		if (auto* programNode = dynamic_cast<ProgramNode*>(node.get())) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] Detected ProgramNode, calling visitProgramNode()\n";
			}
			result = transformer.visitProgramNode(programNode);
		}
		else if (auto* blockNode = dynamic_cast<BlockNode*>(node.get())) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] Detected BlockNode, calling visitBlockNode()\n";
			}
			result = transformer.visitBlockNode(blockNode);
		}
		else if (auto* functionNode = dynamic_cast<FunctionNode*>(node.get())) {
			if (DCE_DEBUG) {
				std::cout << "[DCE] Detected FunctionNode, calling visitFunctionNode()\n";
			}
			result = transformer.visitFunctionNode(functionNode);
		}
		else {
			if (DCE_DEBUG) {
				std::cout << "[DCE] WARNING: Unknown node type, no transformation applied\n";
			}
		}

		auto endTime = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
		setExecutionTime(duration);

		if (DCE_DEBUG) {
			std::cout << "[DCE] Transformation result: " << (result ? "NEW NODE" : "nullptr (using original)") << "\n";
			std::cout << "[DCE] Removed statements: " << removedStatements << "\n";
			std::cout << "[DCE] ===== Dead Code Elimination Pass Complete =====\n\n";
		}

		// If transformation occurred, return transformed node
		// Otherwise, return original node
		if (result) {
			context.setModified(true);
			return result;
		}

		return node;
	}

	std::string DeadCodeEliminationPass::getDescription() const {
		return "Removes unreachable code after return, break, continue, and throw statements";
	}

	void DeadCodeEliminationPass::reportMetrics(OptimizationResult& result) const {
		optimizer::PassMetrics metrics(
			getName(),
			removedStatements,
			getExecutionTime(),
			removedStatements > 0
		);
		result.addPassMetrics(metrics);
	}

	void DeadCodeEliminationPass::reset() {
		OptimizationPass::reset();
		removedStatements = 0;
	}

} // namespace optimizer::passes
