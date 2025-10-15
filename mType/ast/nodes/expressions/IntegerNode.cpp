#include "IntegerNode.hpp"

namespace ast::nodes::expressions
{
    IntegerNode::IntegerNode(int val, const SourceLocation& loc)
    : ASTNode(loc), value(val) {}

    int IntegerNode::getValue() const
    { return value; }

    void IntegerNode::setValue(int val)
    { value = val; }

    Value IntegerNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitIntegerNode(this);
    }

    std::unique_ptr<ASTNode> IntegerNode::clone() const
    {
        return std::make_unique<IntegerNode>(value, location);
    }
}
