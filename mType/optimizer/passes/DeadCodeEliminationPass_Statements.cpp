/**
 * Dead Code Elimination Pass — block/function/loop visitors.
 *
 * Holds DCETransformer visitors that walk straight-line statement
 * containers (Program/Block/Function/Method/Constructor) and loop bodies
 * (If/While/DoWhile/For/ForEach), plus the core transformStatements helper
 * that drops everything after a terminating statement. Branching/cascading
 * visitors live in DeadCodeEliminationPass_Branching.cpp.
 */

#include "DeadCodeEliminationPass.hpp"
#include <cstddef>
#include <cstdint>
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/BlockNode.hpp"
#include "../../ast/nodes/statements/IfNode.hpp"
#include "../../ast/nodes/statements/WhileNode.hpp"
#include "../../ast/nodes/statements/DoWhileNode.hpp"
#include "../../ast/nodes/statements/ForNode.hpp"
#include "../../ast/nodes/statements/ForEachNode.hpp"
#include "../../ast/nodes/statements/BreakNode.hpp"
#include "../../ast/nodes/statements/ContinueNode.hpp"
#include "../../ast/nodes/statements/ThrowNode.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../ast/nodes/functions/ReturnNode.hpp"
#include "../../ast/nodes/classes/MethodNode.hpp"
#include "../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../ast/nodes/classes/SuperConstructorCallNode.hpp"

namespace optimizer::passes
{
    using namespace ast;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;
    using namespace ast::nodes::classes;

    DeadCodeEliminationPass::DCETransformer::DCETransformer(
        base::OptimizationContext* ctx,
        size_t& count)
        : ASTTransformer(ctx)
          , removedCount(count)
    {
    }

    bool DeadCodeEliminationPass::DCETransformer::isTerminatingStatement(ast::ASTNode* node) const
    {
        if (!node)
        {
            return false;
        }

        return dynamic_cast<ReturnNode*>(node) != nullptr ||
            dynamic_cast<BreakNode*>(node) != nullptr ||
            dynamic_cast<ContinueNode*>(node) != nullptr ||
            dynamic_cast<ThrowNode*>(node) != nullptr;
    }

    std::vector<std::unique_ptr<ast::ASTNode>> DeadCodeEliminationPass::DCETransformer::transformStatements(
        const std::vector<std::unique_ptr<ast::ASTNode>>& statements)
    {
        std::vector<std::unique_ptr<ast::ASTNode>> transformedStatements;
        bool foundTerminator = false;
        size_t terminatorIndex = SIZE_MAX;

        for (size_t i = 0; i < statements.size(); ++i)
        {
            if (isTerminatingStatement(statements[i].get()))
            {
                foundTerminator = true;
                terminatorIndex = i;
                break;
            }
        }

        if (foundTerminator && terminatorIndex + 1 < statements.size())
        {
            size_t deadCount = statements.size() - (terminatorIndex + 1);

            removedCount += deadCount;
            modified = true;
            context->recordTransformation("DeadCodeElimination");

            transformedStatements.reserve(terminatorIndex + 1);
            for (size_t i = 0; i <= terminatorIndex; ++i)
            {
                if (statements[i])
                {
                    auto transformed = transformChild(statements[i].get());
                    if (transformed)
                    {
                        transformedStatements.push_back(std::move(transformed));
                    }
                    else
                    {
                        transformedStatements.push_back(statements[i]->clone());
                    }
                }
            }
        }
        else
        {
            transformedStatements.reserve(statements.size());
            for (const auto& stmt : statements)
            {
                if (stmt)
                {
                    auto transformed = transformChild(stmt.get());
                    if (transformed)
                    {
                        transformedStatements.push_back(std::move(transformed));
                    }
                    else
                    {
                        transformedStatements.push_back(stmt->clone());
                    }
                }
            }
        }

        return transformedStatements;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitProgramNode(ProgramNode* node)
    {
        auto transformedStatements = transformStatements(node->getStatements());
        return std::make_unique<ProgramNode>(std::move(transformedStatements), node->getLocation());
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitBlockNode(BlockNode* node)
    {
        auto transformedStatements = transformStatements(node->getStatements());
        return std::make_unique<BlockNode>(std::move(transformedStatements), node->getLocation());
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitFunctionNode(FunctionNode* node)
    {
        auto body = node->getBodyPtr();
        if (!body)
        {
            return nullptr;
        }

        auto transformedBodyUnique = transformChild(body);
        if (!transformedBodyUnique)
        {
            return nullptr;
        }

        std::shared_ptr<ast::ASTNode> transformedBody = std::move(transformedBodyUnique);

        auto transformedFunction = std::make_unique<FunctionNode>(
            node->getName(),
            node->getGenericReturnType(),
            node->getGenericParameters(),
            transformedBody,
            node->getGenericTypeParameters(),
            node->getIsAsync(),
            node->getLocation()
        );

        transformedFunction->setVisibility(node->getVisibility());

        for (const auto& annotation : node->getAnnotations())
        {
            transformedFunction->addAnnotation(annotation);
        }

        // MYT-110: preserve per-parameter annotations through DCE rebuild.
        transformedFunction->setParameterAnnotations(
            std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>>(
                node->getParameterAnnotations()));

        return transformedFunction;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitMethodNode(MethodNode* node)
    {
        auto body = node->getBodyPtr();
        if (!body)
        {
            return nullptr;
        }

        auto transformedBodyUnique = transformChild(body);
        if (!transformedBodyUnique)
        {
            return nullptr;
        }

        std::shared_ptr<ast::ASTNode> transformedBody = std::move(transformedBodyUnique);

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

        transformedMethod->setAbstract(node->isAbstract());
        transformedMethod->setFinal(node->isFinal());
        // MYT-274: preserve synthetic flag through DCE rebuild so the
        // method continues to route into syntheticInstanceMethods at
        // runtime registration (and stays filtered from reflection).
        transformedMethod->setSynthetic(node->isSynthetic());

        for (const auto& annotation : node->getAnnotations())
        {
            transformedMethod->addAnnotation(annotation);
        }

        // MYT-110: preserve per-parameter annotations through DCE rebuild.
        transformedMethod->setParameterAnnotations(
            std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>>(
                node->getParameterAnnotations()));

        return transformedMethod;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitConstructorNode(ConstructorNode* node)
    {
        auto body = node->getBodyPtr();
        if (!body)
        {
            return nullptr;
        }

        auto transformedBodyUnique = transformChild(body);
        if (!transformedBodyUnique)
        {
            return nullptr;
        }

        std::shared_ptr<ast::ASTNode> transformedBody = std::move(transformedBodyUnique);

        auto transformedConstructor = std::make_unique<ConstructorNode>(
            node->getParametersWithTypes(),
            transformedBody,
            node->getAccessModifier(),
            node->getLocation()
        );

        if (node->hasSuperInitializer())
        {
            auto clonedSuper = std::unique_ptr<ast::nodes::classes::SuperConstructorCallNode>(
                static_cast<ast::nodes::classes::SuperConstructorCallNode*>(
                    node->getSuperInitializer()->clone().release()
                )
            );
            transformedConstructor->setSuperInitializer(std::move(clonedSuper));
        }

        // MYT-110: preserve ctor-level and per-parameter annotations through
        // DCE rebuild. Before this change, DCE silently dropped constructor-
        // level annotations when it rebuilt the node.
        for (const auto& annotation : node->getAnnotations())
        {
            transformedConstructor->addAnnotation(annotation);
        }
        transformedConstructor->setParameterAnnotations(
            std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>>(
                node->getParameterAnnotations()));

        return transformedConstructor;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitIfNode(IfNode* node)
    {
        auto transformedCondition = transformChild(node->getCondition());
        auto* conditionToUse = transformedCondition ? transformedCondition.get() : node->getCondition();

        auto transformedThen = transformChild(node->getThenStatement());
        auto* thenToUse = transformedThen ? transformedThen.get() : node->getThenStatement();

        std::unique_ptr<ast::ASTNode> transformedElse = nullptr;
        ASTNode* elseToUse = nullptr;
        if (node->hasElseStatement() && node->getElseStatement())
        {
            transformedElse = transformChild(node->getElseStatement());
            elseToUse = transformedElse ? transformedElse.get() : node->getElseStatement();
        }

        if (transformedCondition || transformedThen || transformedElse)
        {
            return std::make_unique<IfNode>(
                transformedCondition ? std::move(transformedCondition) : conditionToUse->clone(),
                transformedThen ? std::move(transformedThen) : thenToUse->clone(),
                elseToUse ? (transformedElse ? std::move(transformedElse) : elseToUse->clone()) : nullptr,
                node->getLocation()
            );
        }

        return nullptr;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitWhileNode(WhileNode* node)
    {
        auto transformedCondition = transformChild(node->getCondition());
        auto transformedBody = transformChild(node->getBody());

        if (transformedCondition || transformedBody)
        {
            return std::make_unique<WhileNode>(
                transformedCondition ? std::move(transformedCondition) : node->getCondition()->clone(),
                transformedBody ? std::move(transformedBody) : node->getBody()->clone(),
                node->getLocation()
            );
        }

        return nullptr;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitDoWhileNode(DoWhileNode* node)
    {
        auto transformedBody = transformChild(node->getBody());
        auto transformedCondition = transformChild(node->getCondition());

        if (transformedBody || transformedCondition)
        {
            return std::make_unique<DoWhileNode>(
                transformedBody ? std::move(transformedBody) : node->getBody()->clone(),
                transformedCondition ? std::move(transformedCondition) : node->getCondition()->clone(),
                node->getLocation()
            );
        }

        return nullptr;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitForNode(ForNode* node)
    {
        auto transformedInit = node->getInitialization() ? transformChild(node->getInitialization()) : nullptr;
        auto transformedCondition = node->getCondition() ? transformChild(node->getCondition()) : nullptr;
        auto transformedUpdate = node->getUpdate() ? transformChild(node->getUpdate()) : nullptr;
        auto transformedBody = transformChild(node->getBody());

        if (transformedInit || transformedCondition || transformedUpdate || transformedBody)
        {
            return std::make_unique<ForNode>(
                transformedInit
                    ? std::move(transformedInit)
                    : (node->getInitialization() ? node->getInitialization()->clone() : nullptr),
                transformedCondition
                    ? std::move(transformedCondition)
                    : (node->getCondition() ? node->getCondition()->clone() : nullptr),
                transformedUpdate
                    ? std::move(transformedUpdate)
                    : (node->getUpdate() ? node->getUpdate()->clone() : nullptr),
                transformedBody ? std::move(transformedBody) : node->getBody()->clone(),
                node->getLocation()
            );
        }

        return nullptr;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitForEachNode(ForEachNode* node)
    {
        auto transformedCollection = transformChild(node->getCollection());
        auto transformedBody = transformChild(node->getBody());

        if (transformedCollection || transformedBody)
        {
            return std::make_unique<ForEachNode>(
                node->getVariableName(),
                node->getVariableTypeInfo(),
                transformedCollection ? std::move(transformedCollection) : node->getCollection()->clone(),
                transformedBody ? std::move(transformedBody) : node->getBody()->clone(),
                node->getLocation()
            );
        }

        return nullptr;
    }
} // namespace optimizer::passes
