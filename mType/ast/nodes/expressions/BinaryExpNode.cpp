#include "BinaryExpNode.hpp"

namespace ast::nodes::expressions
{
    BinaryExpNode::BinaryExpNode(std::unique_ptr<ASTNode> leftNode, TokenType op, std::unique_ptr<ASTNode> rightNode,
                                 const SourceLocation& loc)
        : ASTNode(loc), left(std::move(leftNode)), right(std::move(rightNode)), operator_(op)
    {
    }

    ASTNode* BinaryExpNode::getLeft() const
    {
        return left.get();
    }

    ASTNode* BinaryExpNode::getRight() const
    {
        return right.get();
    }

    TokenType BinaryExpNode::getOperator() const
    {
        return operator_;
    }

    void BinaryExpNode::setLeft(std::unique_ptr<ASTNode> leftNode)
    {
        left = std::move(leftNode);
    }

    void BinaryExpNode::setRight(std::unique_ptr<ASTNode> rightNode)
    {
        right = std::move(rightNode);
    }

    void BinaryExpNode::setOperator(TokenType op)
    {
        operator_ = op;
    }

    Value BinaryExpNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitBinaryOpNode(this);
    }
}
