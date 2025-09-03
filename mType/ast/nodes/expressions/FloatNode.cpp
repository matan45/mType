#include "FloatNode.hpp"

namespace ast::nodes::expressions
{
    FloatNode::FloatNode(float val, const SourceLocation& loc)
        : ASTNode(loc), value(val)
    {
    }

    float FloatNode::getValue() const
    {
        return value;
    }

    void FloatNode::setValue(float val)
    {
        value = val;
    }

    Value FloatNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitFloatNode(this);
    }
}
