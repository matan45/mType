#include "ContinueNode.hpp"

namespace ast::nodes::statements
{
    ContinueNode::ContinueNode(const SourceLocation& loc)
        : ASTNode(loc)
    {
    }

    Value ContinueNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitContinueNode(this);
    }

    std::unique_ptr<ASTNode> ContinueNode::clone() const
    {
        return std::make_unique<ContinueNode>(location);
    }
}
