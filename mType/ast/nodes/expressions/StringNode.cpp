#include "StringNode.hpp"

namespace ast::nodes::expressions
{
    StringNode::StringNode(const std::string& val, const SourceLocation& loc)
        : ASTNode(loc), value(val)
    {
    }

    const std::string& StringNode::getValue() const
    {
        return value;
    }

    void StringNode::setValue(const std::string& val)
    {
        value = val;
    }

    Value StringNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitStringNode(this);
    }
}
