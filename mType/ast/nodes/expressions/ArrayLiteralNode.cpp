#include "ArrayLiteralNode.hpp"
#include <cstddef>

namespace ast::nodes::expressions
{
    ArrayLiteralNode::ArrayLiteralNode(std::vector<std::unique_ptr<ASTNode>> elems, const SourceLocation& loc)
        : ASTNode(loc), elements(std::move(elems)) {}

    const std::vector<std::unique_ptr<ASTNode>>& ArrayLiteralNode::getElements() const
    {
        return elements;
    }

    size_t ArrayLiteralNode::getElementCount() const
    {
        return elements.size();
    }

    Value ArrayLiteralNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitArrayLiteralNode(this);
    }

    std::unique_ptr<ASTNode> ArrayLiteralNode::clone() const
    {
        std::vector<std::unique_ptr<ASTNode>> clonedElements;
        clonedElements.reserve(elements.size());
        for (const auto& elem : elements) {
            if (elem) {
                clonedElements.push_back(elem->clone());
            }
        }
        return std::make_unique<ArrayLiteralNode>(std::move(clonedElements), location);
    }
}