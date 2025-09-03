#include "VariableNode.hpp"

namespace ast::nodes::expressions
{
    VariableNode::VariableNode(const std::string& varName, const SourceLocation& loc)
        : ASTNode(loc), name(varName)
    {
    }

    const std::string& VariableNode::getName() const
    {
        return name;
    }

    void VariableNode::setName(const std::string& varName)
    {
        name = varName;
    }

    Value VariableNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitVariableNode(this);
    }
}
