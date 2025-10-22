#include "CastExpression.hpp"

namespace ast::nodes::expressions
{
    value::Value CastExpression::accept(ASTVisitor<value::Value>& visitor)
    {
        return visitor.visitCastExpression(this);
    }

    std::unique_ptr<ASTNode> CastExpression::clone() const
    {
        std::unique_ptr<ASTNode> clonedExpr = expression ? expression->clone() : nullptr;
        std::shared_ptr<GenericType> clonedType = targetType ? std::make_shared<GenericType>(*targetType) : nullptr;
        return std::make_unique<CastExpression>(std::move(clonedExpr), clonedType, location);
    }
}
