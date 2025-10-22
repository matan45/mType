#include "InstanceOfExpression.hpp"

namespace ast::nodes::expressions
{
    value::Value InstanceOfExpression::accept(ASTVisitor<value::Value>& visitor)
    {
        return visitor.visitInstanceOfExpression(this);
    }

    std::unique_ptr<ASTNode> InstanceOfExpression::clone() const
    {
        std::unique_ptr<ASTNode> clonedExpr = expression ? expression->clone() : nullptr;
        std::shared_ptr<GenericType> clonedType = targetType ? std::make_shared<GenericType>(*targetType) : nullptr;
        return std::make_unique<InstanceOfExpression>(std::move(clonedExpr), clonedType, location);
    }
}
