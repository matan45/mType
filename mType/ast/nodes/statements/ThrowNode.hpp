#pragma once
#include "../../ASTNode.hpp"
#include <memory>

namespace ast::nodes::statements
{
    /**
     * @brief AST node representing a throw statement
     *
     * Syntax: throw expression;
     * Throws an exception (typically an object instance)
     */
    class ThrowNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> exception;  // Expression that evaluates to the exception to throw

    public:
        explicit ThrowNode(std::unique_ptr<ASTNode> exception,
                          const SourceLocation& loc = SourceLocation());

        ASTNode* getException() const;
        void setException(std::unique_ptr<ASTNode> expr);

        value::Value accept(ASTVisitor<value::Value>& visitor) override;
    };
}
