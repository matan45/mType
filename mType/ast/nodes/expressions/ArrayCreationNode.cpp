#include "ArrayCreationNode.hpp"
#include <stdexcept>

namespace ast::nodes::expressions
{
    // Multi-dimension constructor
    ArrayCreationNode::ArrayCreationNode(const parser::TypeInfo& elemTypeInfo,
                                       std::vector<std::unique_ptr<ASTNode>> sizeExprs,
                                       const errors::SourceLocation& loc)
        : ASTNode(loc), elementTypeInfo(elemTypeInfo), sizeExpressions(std::move(sizeExprs))
    {
        // Initialize dynamic size flags - assume all are dynamic for now
        isDynamicSizes.resize(sizeExpressions.size(), true);
    }

    // Single-dimension constructor (backward compatibility)
    ArrayCreationNode::ArrayCreationNode(const parser::TypeInfo& elemTypeInfo,
                                       std::unique_ptr<ASTNode> sizeExpr,
                                       const errors::SourceLocation& loc)
        : ASTNode(loc), elementTypeInfo(elemTypeInfo)
    {
        sizeExpressions.push_back(std::move(sizeExpr));
        isDynamicSizes.push_back(true);
    }

    const parser::TypeInfo& ArrayCreationNode::getElementTypeInfo() const
    {
        return elementTypeInfo;
    }

    ValueType ArrayCreationNode::getElementType() const
    {
        return elementTypeInfo.baseType;
    }

    const std::vector<std::unique_ptr<ASTNode>>& ArrayCreationNode::getSizeExpressions() const
    {
        return sizeExpressions;
    }

    const std::unique_ptr<ASTNode>& ArrayCreationNode::getSizeExpression() const
    {
        if (sizeExpressions.empty()) {
            throw std::runtime_error("ArrayCreationNode has no size expressions");
        }
        return sizeExpressions[0];
    }

    size_t ArrayCreationNode::getDimensionCount() const
    {
        return sizeExpressions.size();
    }

    bool ArrayCreationNode::hasDynamicSize() const
    {
        for (bool dynamic : isDynamicSizes) {
            if (dynamic) return true;
        }
        return false;
    }

    bool ArrayCreationNode::hasDynamicSize(size_t dimension) const
    {
        if (dimension >= isDynamicSizes.size()) {
            throw std::out_of_range("Dimension index out of range");
        }
        return isDynamicSizes[dimension];
    }

    void ArrayCreationNode::setSizeExpression(std::unique_ptr<ASTNode> sizeExpr)
    {
        sizeExpressions.clear();
        isDynamicSizes.clear();
        sizeExpressions.push_back(std::move(sizeExpr));
        isDynamicSizes.push_back(true);
    }

    void ArrayCreationNode::addDimension(std::unique_ptr<ASTNode> sizeExpr)
    {
        sizeExpressions.push_back(std::move(sizeExpr));
        isDynamicSizes.push_back(true);
    }

    Value ArrayCreationNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitArrayCreationNode(this);
    }
}