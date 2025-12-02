#include "IntegerNode.hpp"

namespace ast::nodes::expressions
{
    IntegerNode::IntegerNode(int64_t val, const SourceLocation& loc)
    : ASTNode(loc), value(val) {}

    int64_t IntegerNode::getValue() const
    { return value; }

    void IntegerNode::setValue(int64_t val)
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
