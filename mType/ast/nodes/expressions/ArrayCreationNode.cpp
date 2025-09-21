#include "ArrayCreationNode.hpp"
#include <stdexcept>

namespace ast::nodes::expressions
{
    ArrayCreationNode::ArrayCreationNode(const parser::TypeInfo& elemTypeInfo,
                                       std::unique_ptr<ASTNode> sizeExpr,
                                       const errors::SourceLocation& loc)
        : ASTNode(loc), elementTypeInfo(elemTypeInfo), sizeExpression(std::move(sizeExpr)), isDynamicSize(true) {}

    const parser::TypeInfo& ArrayCreationNode::getElementTypeInfo() const
    {
        return elementTypeInfo;
    }

    ValueType ArrayCreationNode::getElementType() const
    {
        return elementTypeInfo.baseType;
    }

    const std::unique_ptr<ASTNode>& ArrayCreationNode::getSizeExpression() const
    {
        return sizeExpression;
    }

    bool ArrayCreationNode::hasDynamicSize() const
    {
        return isDynamicSize;
    }

    void ArrayCreationNode::setSizeExpression(std::unique_ptr<ASTNode> sizeExpr)
    {
        sizeExpression = std::move(sizeExpr);
        isDynamicSize = true;
    }

    Value ArrayCreationNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitArrayCreationNode(this);
    }
}