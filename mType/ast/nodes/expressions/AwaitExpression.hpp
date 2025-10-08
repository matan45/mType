#pragma once
#include "../../ASTNode.hpp"
#include <memory>

namespace ast::nodes::expressions
{
    /**
     * @brief Represents an await expression
     *
     * Syntax: await <expression>
     *
     * Examples:
     *   await fetchData()           // Await async function call
     *   await promise               // Await promise value
     *   await getUserById(123)      // Await async operation
     *
     * Design:
     * - The await expression can only appear inside async functions/methods
     * - Suspends execution until the awaited expression completes
     * - The expression must evaluate to an awaitable type (Promise<T>, Task<T>)
     * - Returns the unwrapped result type (Promise<int> -> int)
     *
     * Type System:
     * - await Promise<T> -> T
     * - await Task<T> -> T
     * - Validation enforces await only in async context
     */
    class AwaitExpression : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> expression;  // The expression being awaited

    public:
        explicit AwaitExpression(
            std::unique_ptr<ASTNode> expr,
            const SourceLocation& loc = SourceLocation()
        ) : ASTNode(loc), expression(std::move(expr))
        {
        }

        /**
         * @brief Get the expression being awaited
         * @return Pointer to the awaited expression (non-owning)
         */
        ASTNode* getExpression() const noexcept
        {
            return expression.get();
        }

        /**
         * @brief Set the expression being awaited
         * @param expr The new expression to await
         */
        void setExpression(std::unique_ptr<ASTNode> expr)
        {
            expression = std::move(expr);
        }

        /**
         * @brief Check if this await has a valid expression
         * @return true if expression is not null
         */
        bool hasExpression() const noexcept
        {
            return expression != nullptr;
        }

        /**
         * @brief Accept visitor for AST traversal
         */
        Value accept(ASTVisitor<Value>& visitor) override
        {
            return visitor.visitAwaitExpression(this);
        }
    };
}
