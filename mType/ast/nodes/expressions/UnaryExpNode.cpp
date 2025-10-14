#include "UnaryExpNode.hpp"

namespace ast::nodes::expressions
{
    // Implementation is in header file (inline methods)
    UnaryExpNode::UnaryExpNode(TokenType op, std::unique_ptr<ASTNode> operand_, UnaryPosition pos,
                               const SourceLocation& loc)
        : ASTNode(loc), operator_(op), operand(std::move(operand_)), position(pos)
    {
    }

    TokenType UnaryExpNode::getOperator() const
    {
        return operator_;
    }

    ASTNode* UnaryExpNode::getOperand() const
    {
        return operand.get();
    }

    UnaryPosition UnaryExpNode::getPosition() const
    {
        return position;
    }

    void UnaryExpNode::setOperand(std::unique_ptr<ASTNode> operand_)
    {
        operand = std::move(operand_);
    }

    void UnaryExpNode::setPosition(UnaryPosition pos)
    {
        position = pos;
    }

    bool UnaryExpNode::isPrefix() const
    {
        return position == UnaryPosition::PREFIX;
    }

    bool UnaryExpNode::isPostfix() const
    {
        return position == UnaryPosition::POSTFIX;
    }

    Value UnaryExpNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitUnaryOpNode(this);
    }

    std::unique_ptr<ASTNode> UnaryExpNode::clone() const
    {
        auto clonedOperand = operand ? operand->clone() : nullptr;
        return std::make_unique<UnaryExpNode>(
            operator_,
            std::move(clonedOperand),
            position,
            location
        );
    }
}
