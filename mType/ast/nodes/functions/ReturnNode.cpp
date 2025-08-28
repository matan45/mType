#include "ReturnNode.hpp"

namespace ast::nodes::functions
{
    ReturnNode::ReturnNode(std::unique_ptr<ASTNode> value, const SourceLocation& loc)
        : ASTNode(loc), returnValue(std::move(value))
    {
    }

    ASTNode* ReturnNode::getReturnValue() const
    {
        return returnValue.get();
    }

    void ReturnNode::setReturnValue(std::unique_ptr<ASTNode> value)
    {
        returnValue = std::move(value);
    }

    bool ReturnNode::hasReturnValue() const
    {
        return returnValue != nullptr;
    }

    Value ReturnNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitReturnNode(this);
    }
}
