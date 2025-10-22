#include "TernaryExpNode.hpp"

namespace ast::nodes::expressions
{
    TernaryExpNode::TernaryExpNode(std::unique_ptr<ASTNode> cond, std::unique_ptr<ASTNode> trueExpr,
                                   std::unique_ptr<ASTNode> falseExpr, const SourceLocation& loc)
        : ASTNode(loc), condition(std::move(cond)), trueExpression(std::move(trueExpr)),
          falseExpression(std::move(falseExpr))
    {
    }

    ASTNode* TernaryExpNode::getCondition() const
    {
        return condition.get();
    }

    ASTNode* TernaryExpNode::getTrueExpression() const
    {
        return trueExpression.get();
    }

    ASTNode* TernaryExpNode::getFalseExpression() const
    {
        return falseExpression.get();
    }

    Value TernaryExpNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitTernaryOpNode(this);
    }

    std::unique_ptr<ASTNode> TernaryExpNode::clone() const
    {
        auto clonedCondition = condition ? condition->clone() : nullptr;
        auto clonedTrueExpr = trueExpression ? trueExpression->clone() : nullptr;
        auto clonedFalseExpr = falseExpression ? falseExpression->clone() : nullptr;
        return std::make_unique<TernaryExpNode>(
            std::move(clonedCondition),
            std::move(clonedTrueExpr),
            std::move(clonedFalseExpr),
            location
        );
    }
}
