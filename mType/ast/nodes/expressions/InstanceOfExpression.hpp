#pragma once
#include "../../ASTNode.hpp"
#include "../../GenericType.hpp"
#include <memory>

namespace ast::nodes::expressions
{
    /**
     * @brief Represents isClassOf type checking expression
     *
     * Syntax: expression isClassOf TargetType
     *
     * Examples:
     *   animal isClassOf Dog        // Returns bool
     *   obj isClassOf Drawable      // Interface check
     *   value isClassOf String      // Type check
     *
     * Design:
     * - Always evaluates to boolean
     * - Checks runtime type compatibility
     * - Handles null values (returns false)
     * - Supports interface type checking
     * - Works with inheritance hierarchies
     */
    class InstanceOfExpression : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> expression;
        std::shared_ptr<GenericType> targetType;

    public:
        explicit InstanceOfExpression(
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
            return visitor.visitInstanceOfExpression(this);
        }
    };
}
