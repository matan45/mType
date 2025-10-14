#include "BreakNode.hpp"

namespace ast::nodes::statements
{
    BreakNode::BreakNode(const SourceLocation& loc)
        : ASTNode(loc)
    {
    }

    Value BreakNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitBreakNode(this);
    }

    std::unique_ptr<ASTNode> BreakNode::clone() const
    {
        return std::make_unique<BreakNode>(location);
    }
}
