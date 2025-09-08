#include "ArrayLiteralNode.hpp"

namespace ast::nodes::expressions
{
    ArrayLiteralNode::ArrayLiteralNode(const parser::TypeInfo& elemTypeInfo, const SourceLocation& loc)
        : ASTNode(loc), elementTypeInfo(elemTypeInfo) {}
    
    // Legacy constructor for backward compatibility
    ArrayLiteralNode::ArrayLiteralNode(ValueType elemType, const SourceLocation& loc)
        : ASTNode(loc), elementTypeInfo(parser::TypeInfo(elemType)) {}

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
        return elementTypeInfo.baseType;
    }

    const parser::TypeInfo& ArrayLiteralNode::getElementTypeInfo() const
    {
        return elementTypeInfo;
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