#include "ConstantFoldingPass.hpp"
#include <cstddef>
#include "constant_folding/ConstantEvaluator.hpp"
#include "../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../ast/nodes/expressions/FloatNode.hpp"
#include "../../ast/nodes/expressions/BoolNode.hpp"
#include "../../ast/nodes/expressions/StringNode.hpp"
#include "../../ast/nodes/expressions/NullNode.hpp"
#include "../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../ast/nodes/expressions/CastExpression.hpp"
#include "../../value/StringPool.hpp"

namespace optimizer::passes
{
    using namespace ast;
    using namespace constant_folding;
    using value::Value;
    using value::InternedString;
    using token::TokenType;
    using errors::SourceLocation;

    bool ConstantFoldingPass::CFTransformer::isLiteralNode(ast::ASTNode* node) const
    {
        if (!node) return false;

        return dynamic_cast<ast::nodes::expressions::IntegerNode*>(node) != nullptr ||
            dynamic_cast<ast::nodes::expressions::FloatNode*>(node) != nullptr ||
            dynamic_cast<ast::nodes::expressions::BoolNode*>(node) != nullptr ||
            dynamic_cast<ast::nodes::expressions::StringNode*>(node) != nullptr ||
            dynamic_cast<ast::nodes::expressions::NullNode*>(node) != nullptr;
    }

    std::optional<Value> ConstantFoldingPass::CFTransformer::extractLiteralValue(ast::ASTNode* node)
    {
        if (!node) return std::nullopt;

        if (auto* intNode = dynamic_cast<ast::nodes::expressions::IntegerNode*>(node))
        {
            return Value(intNode->getValue());
        }
        if (auto* floatNode = dynamic_cast<ast::nodes::expressions::FloatNode*>(node))
        {
            return Value(floatNode->getValue());
        }
        if (auto* boolNode = dynamic_cast<ast::nodes::expressions::BoolNode*>(node))
        {
            return Value(boolNode->getValue());
        }
        if (auto* strNode = dynamic_cast<ast::nodes::expressions::StringNode*>(node))
        {
            return Value(strNode->getValue());
        }
        if (dynamic_cast<ast::nodes::expressions::NullNode*>(node))
        {
            return Value(std::monostate{});
        }

        return std::nullopt;
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::createLiteralNode(
        const Value& value,
        const SourceLocation& loc
    )
    {
        if (value::isInt(value))
        {
            return std::make_unique<ast::nodes::expressions::IntegerNode>(value::asInt(value), loc);
        }
        if (value::isFloat(value))
        {
            return std::make_unique<ast::nodes::expressions::FloatNode>(value::asFloat(value), loc);
        }
        if (value::isBool(value))
        {
            return std::make_unique<ast::nodes::expressions::BoolNode>(value::asBool(value), loc);
        }
        if (value::isString(value))
        {
            return std::make_unique<ast::nodes::expressions::StringNode>(value::asString(value), loc);
        }
        if (value::isInternedString(value))
        {
            return std::make_unique<ast::nodes::expressions::StringNode>(
                value::asInternedString(value).getString(), loc);
        }
        if (value::isVoid(value) ||
            value::isNullType(value))
        {
            return std::make_unique<ast::nodes::expressions::NullNode>(loc);
        }

        return nullptr;
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::tryFoldBinaryOp(
        BinaryOpNode* node
    )
    {
        auto leftChild = transformChild(node->getLeft());
        auto rightChild = transformChild(node->getRight());

        auto leftValue = extractLiteralValue(leftChild.get());
        auto rightValue = extractLiteralValue(rightChild.get());

        if (leftValue && rightValue)
        {
            auto result = ConstantEvaluator::evaluateBinaryOp(
                *leftValue,
                *rightValue,
                node->getOperator()
            );

            if (result)
            {
                auto foldedNode = createLiteralNode(*result, node->getLocation());
                if (foldedNode)
                {
                    foldedExpressionsCount++;
                    modified = true;

                    return foldedNode;
                }
            }
        }

        auto newNode = std::make_unique<ast::nodes::expressions::BinaryExpNode>(
            std::move(leftChild),
            node->getOperator(),
            std::move(rightChild),
            node->getLocation()
        );
        return newNode;
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::tryFoldUnaryOp(
        UnaryOpNode* node
    )
    {
        // Don't fold increment/decrement: they have side effects.
        if (node->getOperator() == TokenType::INCREMENT ||
            node->getOperator() == TokenType::DECREMENT)
        {
            auto operand = transformChild(node->getOperand());
            auto newNode = std::make_unique<ast::nodes::expressions::UnaryExpNode>(
                node->getOperator(),
                std::move(operand),
                node->getPosition(),
                node->getLocation()
            );
            return newNode;
        }

        auto operandChild = transformChild(node->getOperand());

        auto operandValue = extractLiteralValue(operandChild.get());

        if (operandValue)
        {
            auto result = ConstantEvaluator::evaluateUnaryOp(
                *operandValue,
                node->getOperator()
            );

            if (result)
            {
                auto foldedNode = createLiteralNode(*result, node->getLocation());
                if (foldedNode)
                {
                    foldedExpressionsCount++;
                    modified = true;

                    return foldedNode;
                }
            }
        }

        auto newNode = std::make_unique<ast::nodes::expressions::UnaryExpNode>(
            node->getOperator(),
            std::move(operandChild),
            node->getPosition(),
            node->getLocation()
        );
        return newNode;
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::tryFoldTernary(
        TernaryOpNode* node
    )
    {
        auto condition = transformChild(node->getCondition());

        auto conditionValue = extractLiteralValue(condition.get());

        if (conditionValue && value::isBool(*conditionValue))
        {
            bool condBool = value::asBool(*conditionValue);

            if (condBool)
            {
                auto trueExpr = transformChild(node->getTrueExpression());
                foldedExpressionsCount++;
                modified = true;
                return trueExpr;
            }
            else
            {
                auto falseExpr = transformChild(node->getFalseExpression());
                foldedExpressionsCount++;
                modified = true;
                return falseExpr;
            }
        }

        auto trueExpr = transformChild(node->getTrueExpression());
        auto falseExpr = transformChild(node->getFalseExpression());

        auto newNode = std::make_unique<ast::nodes::expressions::TernaryExpNode>(
            std::move(condition),
            std::move(trueExpr),
            std::move(falseExpr),
            node->getLocation()
        );
        return newNode;
    }

    std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::tryFoldCast(
        CastExpression* node
    )
    {
        auto expr = transformChild(node->getExpression());

        auto exprValue = extractLiteralValue(expr.get());

        // Only fold primitive type casts.
        if (exprValue && node->getTargetType())
        {
            const auto* targetType = node->getTargetType();

            ValueType targetValueType;
            bool isPrimitive = false;

            if (targetType->getBaseTypeName() == "int")
            {
                targetValueType = ValueType::INT;
                isPrimitive = true;
            }
            else if (targetType->getBaseTypeName() == "float")
            {
                targetValueType = ValueType::FLOAT;
                isPrimitive = true;
            }
            else if (targetType->getBaseTypeName() == "bool")
            {
                targetValueType = ValueType::BOOL;
                isPrimitive = true;
            }
            else if (targetType->getBaseTypeName() == "String")
            {
                targetValueType = ValueType::STRING;
                isPrimitive = true;
            }

            if (isPrimitive)
            {
                auto result = ConstantEvaluator::evaluateTypeCast(*exprValue, targetValueType);

                if (result)
                {
                    auto foldedNode = createLiteralNode(*result, node->getLocation());
                    if (foldedNode)
                    {
                        foldedExpressionsCount++;
                        modified = true;

                        return foldedNode;
                    }
                }
            }
        }

        auto newNode = std::make_unique<ast::nodes::expressions::CastExpression>(
            std::move(expr),
            node->getTargetTypeShared(),
            node->getLocation()
        );
        return newNode;
    }
}
