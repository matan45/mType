#include "AwaitExpression.hpp"

namespace ast::nodes::expressions
{
    value::Value AwaitExpression::accept(ASTVisitor<value::Value>& visitor)
    {
        return visitor.visitAwaitExpression(this);
    }

    std::unique_ptr<ASTNode> AwaitExpression::clone() const
    {
        std::unique_ptr<ASTNode> clonedExpr = expression ? expression->clone() : nullptr;
        return std::make_unique<AwaitExpression>(std::move(clonedExpr), location);
    }
}
