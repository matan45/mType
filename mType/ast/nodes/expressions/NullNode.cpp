#include "NullNode.hpp"

namespace ast::nodes::expressions
{
    NullNode::NullNode(const SourceLocation& loc)
        : ASTNode(loc)
    {
    }

    Value NullNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitNullNode(this);
    }
}
