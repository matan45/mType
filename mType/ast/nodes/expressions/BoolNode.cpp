#include "BoolNode.hpp"

namespace ast::nodes::expressions
{
    BoolNode::BoolNode(bool val, const SourceLocation& loc)
        : ASTNode(loc), value(val)
    {
    }

    bool BoolNode::getValue() const
    {
        return value;
    }

    void BoolNode::setValue(bool val)
    {
        value = val;
    }

    Value BoolNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitBoolNode(this);
    }

    std::unique_ptr<ASTNode> BoolNode::clone() const
    {
        return std::make_unique<BoolNode>(value, location);
    }
}
