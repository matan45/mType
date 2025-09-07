#include "ArrayLiteralNode.hpp"

namespace ast::nodes::expressions
{
    ArrayLiteralNode::ArrayLiteralNode(ValueType elemType, const SourceLocation& loc)
        : ASTNode(loc), elementType(elemType) {}

    void ArrayLiteralNode::addElement(std::unique_ptr<ASTNode> element)
    {
        elements.push_back(std::move(element));
    }

    const std::vector<std::unique_ptr<ASTNode>>& ArrayLiteralNode::getElements() const
    {
        return elements;
    }

    ValueType ArrayLiteralNode::getElementType() const
    {
        return elementType;
    }

    size_t ArrayLiteralNode::getElementCount() const
    {
        return elements.size();
    }

    Value ArrayLiteralNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitArrayLiteralNode(this);
    }
}