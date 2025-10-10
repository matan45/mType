#include "ThrowNode.hpp"

namespace ast::nodes::statements
{
    ThrowNode::ThrowNode(std::unique_ptr<ASTNode> exception, const SourceLocation& loc)
        : ASTNode(loc), exception(std::move(exception))
    {
    }

    ASTNode* ThrowNode::getException() const
    {
        return exception.get();
    }

    void ThrowNode::setException(std::unique_ptr<ASTNode> expr)
    {
        exception = std::move(expr);
    }

    value::Value ThrowNode::accept(ASTVisitor<value::Value>& visitor)
    {
        return visitor.visitThrowNode(this);
    }
}
