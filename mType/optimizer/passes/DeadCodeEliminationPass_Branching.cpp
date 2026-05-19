/**
 * Dead Code Elimination Pass — branching/cascading visitors.
 *
 * Holds DCETransformer visitors for nodes whose dead code lives inside
 * cases, catches, lambdas, classes, and assigned lambda values:
 * Switch/Case/DefaultCase/Match/MatchCase/MatchDefault/Try/Catch/Lambda/
 * Class/Assignment. visitClassNode's child-list rewrite is factored into
 * transformClassChildren + populateRebuiltClass for the 50-line cap.
 *
 * Driver, isTerminatingStatement, transformStatements, and straight-line
 * visitors live in the sibling _Statements.cpp / main .cpp files.
 */

#include "DeadCodeEliminationPass.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../ast/nodes/statements/SwitchNode.hpp"
#include "../../ast/nodes/statements/CaseNode.hpp"
#include "../../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../../ast/nodes/statements/TryNode.hpp"
#include "../../ast/nodes/statements/CatchNode.hpp"
#include "../../ast/nodes/statements/MatchNode.hpp"
#include "../../ast/nodes/statements/MatchCaseNode.hpp"
#include "../../ast/nodes/statements/MatchDefaultNode.hpp"
#include "../../ast/nodes/expressions/LambdaNode.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"

namespace optimizer::passes
{
    using namespace ast;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::classes;

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitSwitchNode(SwitchNode* node)
    {
        auto transformedExpr = transformChild(node->getExpression());

        bool anyCaseTransformed = false;
        std::vector<std::unique_ptr<ast::ASTNode>> transformedCases;
        transformedCases.reserve(node->getCases().size());

        for (const auto& caseNode : node->getCases())
        {
            auto transformed = transformChild(caseNode.get());
            if (transformed)
            {
                anyCaseTransformed = true;
                transformedCases.push_back(std::move(transformed));
            }
            else
            {
                transformedCases.push_back(caseNode->clone());
            }
        }

        if (transformedExpr || anyCaseTransformed)
        {
            auto newSwitch = std::make_unique<SwitchNode>(
                transformedExpr ? std::move(transformedExpr) : node->getExpression()->clone(),
                node->getLocation()
            );

            for (auto& caseNode : transformedCases)
            {
                newSwitch->addCase(std::move(caseNode));
            }

            return newSwitch;
        }

        return nullptr;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitCaseNode(CaseNode* node)
    {
        auto transformedValue = transformChild(node->getValue());

        size_t originalSize = node->getStatements().size();
        auto transformedStatements = transformStatements(node->getStatements());

        bool hasChanges = transformedValue != nullptr || transformedStatements.size() != originalSize;
        if (!hasChanges)
        {
            return nullptr;
        }

        auto newCase = std::make_unique<CaseNode>(
            transformedValue ? std::move(transformedValue) : node->getValue()->clone(),
            node->getLocation()
        );

        for (auto& stmt : transformedStatements)
        {
            newCase->addStatement(std::move(stmt));
        }

        return newCase;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitDefaultCaseNode(DefaultCaseNode* node)
    {
        size_t originalSize = node->getStatements().size();
        auto transformedStatements = transformStatements(node->getStatements());

        if (transformedStatements.size() == originalSize)
        {
            return nullptr;
        }

        auto newDefaultCase = std::make_unique<DefaultCaseNode>(node->getLocation());

        for (auto& stmt : transformedStatements)
        {
            newDefaultCase->addStatement(std::move(stmt));
        }

        return newDefaultCase;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitMatchNode(MatchNode* node)
    {
        auto transformedExpr = transformChild(node->getExpression());

        bool anyCaseTransformed = false;
        std::vector<std::unique_ptr<ast::ASTNode>> transformedCases;
        transformedCases.reserve(node->getCases().size());

        for (const auto& caseNode : node->getCases())
        {
            auto transformed = transformChild(caseNode.get());
            if (transformed)
            {
                anyCaseTransformed = true;
                transformedCases.push_back(std::move(transformed));
            }
            else
            {
                transformedCases.push_back(caseNode->clone());
            }
        }

        if (transformedExpr || anyCaseTransformed)
        {
            auto newMatch = std::make_unique<MatchNode>(
                transformedExpr ? std::move(transformedExpr) : node->getExpression()->clone(),
                node->getLocation()
            );
            for (auto& caseNode : transformedCases)
            {
                newMatch->addCase(std::move(caseNode));
            }
            return newMatch;
        }

        return nullptr;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitMatchCaseNode(MatchCaseNode* node)
    {
        auto transformedBody = transformChild(node->getBody());
        if (transformedBody)
        {
            auto cloned = node->clone();
            auto* matchCase = dynamic_cast<MatchCaseNode*>(cloned.get());
            if (matchCase)
            {
                matchCase->setBody(std::move(transformedBody));
            }
            return cloned;
        }
        return nullptr;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitMatchDefaultNode(MatchDefaultNode* node)
    {
        auto transformedBody = transformChild(node->getBody());
        if (transformedBody)
        {
            return std::make_unique<MatchDefaultNode>(std::move(transformedBody), node->getLocation());
        }
        return nullptr;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitTryNode(TryNode* node)
    {
        auto transformedTry = transformChild(node->getTryBlock());

        bool anyCatchTransformed = false;
        std::vector<std::unique_ptr<CatchNode>> transformedCatches;
        transformedCatches.reserve(node->getCatchBlocks().size());

        for (const auto& catchBlock : node->getCatchBlocks())
        {
            auto transformed = transformChild(catchBlock.get());
            if (transformed)
            {
                anyCatchTransformed = true;
                transformedCatches.push_back(std::unique_ptr<CatchNode>(
                    static_cast<CatchNode*>(transformed.release())
                ));
            }
            else
            {
                transformedCatches.push_back(std::unique_ptr<CatchNode>(
                    static_cast<CatchNode*>(catchBlock->clone().release())
                ));
            }
        }

        std::unique_ptr<ast::ASTNode> transformedFinally = nullptr;
        if (node->hasFinallyBlock())
        {
            transformedFinally = transformChild(node->getFinallyBlock());
        }

        if (transformedTry || anyCatchTransformed || transformedFinally)
        {
            return std::make_unique<TryNode>(
                transformedTry ? std::move(transformedTry) : node->getTryBlock()->clone(),
                std::move(transformedCatches),
                transformedFinally
                    ? std::move(transformedFinally)
                    : (node->hasFinallyBlock() ? node->getFinallyBlock()->clone() : nullptr),
                node->getLocation()
            );
        }

        return nullptr;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitCatchNode(CatchNode* node)
    {
        auto body = node->getBody();
        if (!body)
        {
            return nullptr;
        }

        auto transformedBody = transformChild(body);
        if (!transformedBody)
        {
            return nullptr;
        }

        return std::make_unique<CatchNode>(
            node->getExceptionType(),
            node->getVariableName(),
            std::move(transformedBody),
            node->getLocation()
        );
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitLambdaNode(
        ast::nodes::expressions::LambdaNode* node)
    {
        // Only block lambdas can contain dead code; expression lambdas cannot.
        if (!node->isBlockLambda())
        {
            return nullptr;
        }

        auto body = node->getBody();
        if (!body)
        {
            return nullptr;
        }

        auto transformedBody = transformChild(body);
        if (!transformedBody)
        {
            return nullptr;
        }

        return std::make_unique<ast::nodes::expressions::LambdaNode>(
            node->getParameters(),
            std::move(transformedBody),
            node->getLocation(),
            node->getBodyType(),
            node->getIsAsync()
        );
    }

    std::vector<std::unique_ptr<ast::ASTNode>>
    DeadCodeEliminationPass::DCETransformer::transformClassChildren(
        const std::vector<std::unique_ptr<ast::ASTNode>>& children,
        bool& anyTransformed)
    {
        std::vector<std::unique_ptr<ast::ASTNode>> result;
        result.reserve(children.size());

        for (const auto& child : children)
        {
            auto transformed = transformChild(child.get());
            if (transformed)
            {
                anyTransformed = true;
                result.push_back(std::move(transformed));
            }
            else
            {
                result.push_back(child->clone());
            }
        }
        return result;
    }

    void DeadCodeEliminationPass::DCETransformer::populateRebuiltClass(
        ast::nodes::classes::ClassNode* newClass,
        ast::nodes::classes::ClassNode* original,
        std::vector<std::unique_ptr<ast::ASTNode>>&& constructors,
        std::vector<std::unique_ptr<ast::ASTNode>>&& methods)
    {
        // Fields don't have bodies to DCE — clone as-is.
        for (const auto& field : original->getFields())
        {
            if (field)
            {
                newClass->addField(field->clone());
            }
        }

        for (auto& constructor : constructors)
        {
            newClass->addConstructor(std::move(constructor));
        }
        for (auto& method : methods)
        {
            newClass->addMethod(std::move(method));
        }

        newClass->setFinal(original->isFinal());
        newClass->setAbstract(original->isAbstract());
        newClass->setValueClass(original->isValueClass());
        newClass->setVisibility(original->getVisibility());

        for (const auto& annotation : original->getAnnotations())
        {
            newClass->addAnnotation(annotation);
        }
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitClassNode(
        ast::nodes::classes::ClassNode* node)
    {
        bool anyConstructorTransformed = false;
        auto transformedConstructors = transformClassChildren(
            node->getConstructors(), anyConstructorTransformed);

        bool anyMethodTransformed = false;
        auto transformedMethods = transformClassChildren(
            node->getMethods(), anyMethodTransformed);

        if (!anyConstructorTransformed && !anyMethodTransformed)
        {
            return nullptr;
        }

        // Inheritance ctor preserves extends/implements.
        auto newClass = std::make_unique<ast::nodes::classes::ClassNode>(
            node->getClassName(),
            node->getGenericParameters(),
            node->getParentClassName(),
            node->getImplementedInterfaces(),
            node->getLocation()
        );

        populateRebuiltClass(
            newClass.get(), node,
            std::move(transformedConstructors),
            std::move(transformedMethods));

        return newClass;
    }

    std::unique_ptr<ast::ASTNode> DeadCodeEliminationPass::DCETransformer::visitAssignmentNode(
        ast::nodes::statements::AssignmentNode* node)
    {
        auto value = node->getValue();
        if (!value)
        {
            return nullptr;
        }

        // Triggers visitLambdaNode when the assigned value is a lambda.
        auto transformedValue = transformChild(value);
        if (!transformedValue)
        {
            return nullptr;
        }

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
} // namespace optimizer::passes
