#pragma once
#include "../../ASTNode.hpp"
#include "../../GenericType.hpp"
#include <memory>

namespace ast::nodes::expressions
{
    /**
     * @brief Represents a type cast expression
     *
     * Syntax: (TargetType)expression
     *
     * Examples:
     *   (Dog)animal          // Cast to Dog type
     *   (int)3.14           // Cast float to int
     *   (Drawable)circle    // Cast to interface
     *
     * Design:
     * - Supports both object and primitive type casting
     * - Integrates with GenericType for full type support
     * - Runtime type safety checked during evaluation
     */
    class CastExpression : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> expression;
        std::shared_ptr<GenericType> targetType;

    public:
        explicit CastExpression(
            std::unique_ptr<ASTNode> expr,
            std::shared_ptr<GenericType> type,
            const SourceLocation& loc = SourceLocation()
        ) : ASTNode(loc), expression(std::move(expr)), targetType(std::move(type))
        {
        }

        ASTNode* getExpression() const noexcept
        {
            return expression.get();
        }

        const GenericType* getTargetType() const noexcept
        {
            return targetType.get();
        }

        std::shared_ptr<GenericType> getTargetTypeShared() const noexcept
        {
            return targetType;
        }

        void setExpression(std::unique_ptr<ASTNode> expr)
        {
            expression = std::move(expr);
        }

        void setTargetType(std::shared_ptr<GenericType> type)
        {
            targetType = std::move(type);
        }

        Value accept(ASTVisitor<Value>& visitor) override
        {
            return visitor.visitCastExpression(this);
        }
    };
}
