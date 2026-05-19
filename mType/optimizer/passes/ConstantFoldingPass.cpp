#include "ConstantFoldingPass.hpp"
#include <chrono>
#include <cstddef>
#include "../OptimizationResult.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/BlockNode.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"

namespace optimizer::passes
{
    using namespace ast;
    using errors::SourceLocation;

    ConstantFoldingPass::CFTransformer::CFTransformer(
        base::OptimizationContext* ctx,
        size_t& count
    ) : ASTTransformer(ctx), foldedExpressionsCount(count)
    {
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitBinaryOpNode(
        BinaryOpNode* node
    )
    {
        return tryFoldBinaryOp(node);
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitUnaryOpNode(
        UnaryOpNode* node
    )
    {
        return tryFoldUnaryOp(node);
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitTernaryOpNode(
        TernaryOpNode* node
    )
    {
        return tryFoldTernary(node);
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitCastExpression(
        CastExpression* node
    )
    {
        return tryFoldCast(node);
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitProgramNode(
        ProgramNode* node
    )
    {
        std::vector<std::unique_ptr<ast::ASTNode>> transformedStatements;
        transformedStatements.reserve(node->getStatements().size());

        for (const auto& stmt : node->getStatements())
        {
            auto transformed = transformChild(stmt.get());
            transformedStatements.push_back(std::move(transformed));
        }

        // Always return transformed tree to preserve cloned state, even if no folding occurred.
        return std::make_unique<ProgramNode>(std::move(transformedStatements), node->getLocation());
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitBlockNode(
        BlockNode* node
    )
    {
        std::vector<std::unique_ptr<ast::ASTNode>> transformedStatements;
        transformedStatements.reserve(node->getStatements().size());

        for (const auto& stmt : node->getStatements())
        {
            auto transformed = transformChild(stmt.get());
            transformedStatements.push_back(std::move(transformed));
        }

        return std::make_unique<BlockNode>(std::move(transformedStatements), node->getLocation());
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitFunctionNode(
        FunctionNode* node
    )
    {
        auto body = node->getBodyPtr();
        if (!body)
        {
            return nullptr;
        }

        size_t foldedBefore = foldedExpressionsCount;

        auto transformedBody = transformChild(body);

        if (foldedExpressionsCount > foldedBefore)
        {
            std::shared_ptr<ast::ASTNode> transformedBodyShared = std::move(transformedBody);

            auto transformedFunction = std::make_unique<FunctionNode>(
                node->getName(),
                node->getGenericReturnType(),
                node->getGenericParameters(),
                transformedBodyShared,
                node->getGenericTypeParameters(),
                node->getIsAsync(),
                node->getLocation()
            );

            transformedFunction->setVisibility(node->getVisibility());

            for (const auto& annotation : node->getAnnotations())
            {
                transformedFunction->addAnnotation(annotation);
            }

            // Preserve per-parameter annotations through folding.
            transformedFunction->setParameterAnnotations(
                std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>>(
                    node->getParameterAnnotations()));

            return transformedFunction;
        }

        return nullptr;
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitMethodNode(
        MethodNode* node
    )
    {
        return ASTTransformer::visitMethodNode(node);
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitConstructorNode(
        ConstructorNode* node
    )
    {
        return ASTTransformer::visitConstructorNode(node);
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitIfNode(
        IfNode* node
    )
    {
        return ASTTransformer::visitIfNode(node);
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitWhileNode(
        WhileNode* node
    )
    {
        return ASTTransformer::visitWhileNode(node);
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitForNode(
        ForNode* node
    )
    {
        return ASTTransformer::visitForNode(node);
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitAssignmentNode(
        AssignmentNode* node
    )
    {
        auto value = node->getValue();
        if (!value)
        {
            return nullptr;
        }

        size_t foldedBefore = foldedExpressionsCount;

        auto transformedValue = transformChild(value);

        if (foldedExpressionsCount > foldedBefore)
        {
            auto newAssignment = std::make_unique<ast::nodes::statements::AssignmentNode>(
                node->getVariableName(),
                std::move(transformedValue),
                node->getVariableType(),
                node->getClassName(),
                node->getIsFinal(),
                node->getIsStatic(),
                node->getLocation()
            );

            newAssignment->setVisibility(node->getVisibility());
            newAssignment->setNullableType(node->isNullableType());
            return newAssignment;
        }

        return nullptr;
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitReturnNode(
        ReturnNode* node
    )
    {
        return ASTTransformer::visitReturnNode(node);
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitClassNode(
        ClassNode* node
    )
    {
        return ASTTransformer::visitClassNode(node);
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitArrayLiteralNode(
        ArrayLiteralNode* node
    )
    {
        return ASTTransformer::visitArrayLiteralNode(node);
    }

    ConstantFoldingPass::ConstantFoldingPass()
        : OptimizationPass("ConstantFolding", base::PassType::TRANSFORMATION),
          foldedExpressions(0)
    {
        dependencies = {};
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::optimize(
        std::unique_ptr<ast::ASTNode> node,
        base::OptimizationContext& context
    )
    {
        foldedExpressions = 0;
        transformationCount = 0;

        auto startTime = std::chrono::high_resolution_clock::now();

        CFTransformer transformer(&context, foldedExpressions);

        // Manual dispatch — check node type and call the appropriate visit method.
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
        executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        transformationCount = foldedExpressions;

        if (transformer.wasModified())
        {
            context.setModified(true);
        }

        if (result)
        {
            return result;
        }

        return node;
    }

    std::string ConstantFoldingPass::getDescription() const
    {
        return "Constant Folding: Evaluates and folds constant expressions at compile-time";
    }

    void ConstantFoldingPass::reportMetrics(OptimizationResult& result) const
    {
        PassMetrics metrics;
        metrics.passName = getName();
        metrics.transformationsApplied = foldedExpressions;
        metrics.executionTime = executionTime;
        metrics.modified = (foldedExpressions > 0);

        result.addPassMetrics(metrics);
    }

    void ConstantFoldingPass::reset()
    {
        foldedExpressions = 0;
        transformationCount = 0;
        executionTime = std::chrono::milliseconds(0);
    }
}
