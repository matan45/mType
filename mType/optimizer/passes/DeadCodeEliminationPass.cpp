/**
 * Dead Code Elimination Pass — driver + node-count utility.
 *
 * Removes unreachable code after return/break/continue/throw by walking the
 * AST and rebuilding nodes with dead-tail statements dropped. The visitor
 * surface is split across sibling translation units to stay under the
 * 500-line per-.cpp cap:
 *   - DeadCodeEliminationPass_Statements.cpp — block/function/loop visitors
 *   - DeadCodeEliminationPass_Branching.cpp  — case/catch/lambda/class visitors
 *
 * The free function `countNodes` is exported (Optimizer.cpp forward-
 * declares it as extern), so it must stay external-linkage in this TU.
 */

#include "DeadCodeEliminationPass.hpp"
#include <cstddef>
#include <chrono>
#include "../OptimizationResult.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/BlockNode.hpp"
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
#include "../../ast/nodes/expressions/LambdaNode.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../ast/nodes/classes/MethodNode.hpp"
#include "../../ast/nodes/classes/ConstructorNode.hpp"

namespace optimizer::passes
{
    using namespace ast;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;
    using namespace ast::nodes::classes;

    // ================= Node Counting Utility =================

    // Forward-declare so the anonymous-namespace per-category helpers below
    // can recurse through the public dispatcher.
    size_t countNodes(const ast::ASTNode* node);

    namespace
    {
        size_t countSequenceContainerChildren(const ast::ASTNode* node)
        {
            if (auto* programNode = dynamic_cast<const ProgramNode*>(node))
            {
                size_t sum = 0;
                for (const auto& stmt : programNode->getStatements())
                {
                    sum += countNodes(stmt.get());
                }
                return sum;
            }
            if (auto* blockNode = dynamic_cast<const BlockNode*>(node))
            {
                size_t sum = 0;
                for (const auto& stmt : blockNode->getStatements())
                {
                    sum += countNodes(stmt.get());
                }
                return sum;
            }
            return 0;
        }

        size_t countFunctionLikeChildren(const ast::ASTNode* node)
        {
            if (auto* functionNode = dynamic_cast<const FunctionNode*>(node))
            {
                return countNodes(functionNode->getBodyPtr());
            }
            if (auto* methodNode = dynamic_cast<const MethodNode*>(node))
            {
                return countNodes(methodNode->getBodyPtr());
            }
            if (auto* constructorNode = dynamic_cast<const ConstructorNode*>(node))
            {
                return countNodes(constructorNode->getBodyPtr());
            }
            return 0;
        }

        size_t countConditionalChildren(const ast::ASTNode* node)
        {
            if (auto* ifNode = dynamic_cast<const IfNode*>(node))
            {
                size_t sum = countNodes(ifNode->getCondition())
                    + countNodes(ifNode->getThenStatement());
                if (ifNode->hasElseStatement())
                {
                    sum += countNodes(ifNode->getElseStatement());
                }
                return sum;
            }
            if (auto* whileNode = dynamic_cast<const WhileNode*>(node))
            {
                return countNodes(whileNode->getCondition())
                    + countNodes(whileNode->getBody());
            }
            if (auto* doWhileNode = dynamic_cast<const DoWhileNode*>(node))
            {
                return countNodes(doWhileNode->getBody())
                    + countNodes(doWhileNode->getCondition());
            }
            if (auto* forNode = dynamic_cast<const ForNode*>(node))
            {
                return countNodes(forNode->getInitialization())
                    + countNodes(forNode->getCondition())
                    + countNodes(forNode->getUpdate())
                    + countNodes(forNode->getBody());
            }
            if (auto* forEachNode = dynamic_cast<const ForEachNode*>(node))
            {
                return countNodes(forEachNode->getCollection())
                    + countNodes(forEachNode->getBody());
            }
            return 0;
        }

        size_t countCaseGroupChildren(const ast::ASTNode* node)
        {
            if (auto* switchNode = dynamic_cast<const SwitchNode*>(node))
            {
                size_t sum = countNodes(switchNode->getExpression());
                for (const auto& caseNode : switchNode->getCases())
                {
                    sum += countNodes(caseNode.get());
                }
                return sum;
            }
            if (auto* caseNode = dynamic_cast<const CaseNode*>(node))
            {
                size_t sum = countNodes(caseNode->getValue());
                for (const auto& stmt : caseNode->getStatements())
                {
                    sum += countNodes(stmt.get());
                }
                return sum;
            }
            if (auto* defaultCaseNode = dynamic_cast<const DefaultCaseNode*>(node))
            {
                size_t sum = 0;
                for (const auto& stmt : defaultCaseNode->getStatements())
                {
                    sum += countNodes(stmt.get());
                }
                return sum;
            }
            return 0;
        }

        size_t countExceptionalChildren(const ast::ASTNode* node)
        {
            if (auto* tryNode = dynamic_cast<const TryNode*>(node))
            {
                size_t sum = countNodes(tryNode->getTryBlock());
                for (const auto& catchBlock : tryNode->getCatchBlocks())
                {
                    sum += countNodes(catchBlock.get());
                }
                if (tryNode->hasFinallyBlock())
                {
                    sum += countNodes(tryNode->getFinallyBlock());
                }
                return sum;
            }
            if (auto* catchNode = dynamic_cast<const CatchNode*>(node))
            {
                return countNodes(catchNode->getBody());
            }
            if (auto* lambdaNode = dynamic_cast<const ast::nodes::expressions::LambdaNode*>(node))
            {
                return countNodes(lambdaNode->getBody());
            }
            return 0;
        }
    } // namespace

    // Recursively count all nodes in the AST (extern'd by optimizer/Optimizer.cpp).
    size_t countNodes(const ast::ASTNode* node)
    {
        if (!node)
        {
            return 0;
        }
        return 1
            + countSequenceContainerChildren(node)
            + countFunctionLikeChildren(node)
            + countConditionalChildren(node)
            + countCaseGroupChildren(node)
            + countExceptionalChildren(node);
    }

    // ================= DeadCodeEliminationPass Implementation =================

    DeadCodeEliminationPass::DeadCodeEliminationPass()
        : OptimizationPass("DeadCodeElimination", base::PassType::TRANSFORMATION)
          , removedStatements(0)
    {
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::optimize(
        std::unique_ptr<ast::ASTNode> node,
        base::OptimizationContext& context)
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        DCETransformer transformer(&context, removedStatements);

        // Manual dispatch by root node type.
        std::unique_ptr<ast::ASTNode> result;
        if (auto* programNode = dynamic_cast<ProgramNode*>(node.get()))
        {
            result = transformer.visitProgramNode(programNode);
        }
        else if (auto* blockNode = dynamic_cast<BlockNode*>(node.get()))
        {
            result = transformer.visitBlockNode(blockNode);
        }
        else if (auto* functionNode = dynamic_cast<FunctionNode*>(node.get()))
        {
            result = transformer.visitFunctionNode(functionNode);
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        setExecutionTime(duration);

        if (result)
        {
            // Only mark context modified when we actually removed dead code.
            if (transformer.wasModified())
            {
                context.setModified(true);
            }
            return result;
        }

        return node;
    }

    std::string DeadCodeEliminationPass::getDescription() const
    {
        return "Removes unreachable code after return, break, continue, and throw statements";
    }

    void DeadCodeEliminationPass::reportMetrics(OptimizationResult& result) const
    {
        optimizer::PassMetrics metrics(
            getName(),
            removedStatements,
            getExecutionTime(),
            removedStatements > 0
        );
        result.addPassMetrics(metrics);
    }

    void DeadCodeEliminationPass::reset()
    {
        OptimizationPass::reset();
        removedStatements = 0;
    }
} // namespace optimizer::passes
