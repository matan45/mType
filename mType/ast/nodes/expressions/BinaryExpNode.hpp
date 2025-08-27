#pragma once
#include "../../ASTNode.hpp"
#include "../../../token/TokenType.hpp"
#include <memory>

namespace ast::nodes::expressions
{
    using namespace token;

    class BinaryExpNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> left;
        std::unique_ptr<ASTNode> right;
        TokenType operator_;

    public:
        explicit BinaryExpNode(std::unique_ptr<ASTNode> leftNode, TokenType op, std::unique_ptr<ASTNode> rightNode, 
                      const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), left(std::move(leftNode)), right(std::move(rightNode)), operator_(op) {}

        ASTNode* getLeft() const { return left.get(); }
        ASTNode* getRight() const { return right.get(); }
        TokenType getOperator() const { return operator_; }

        void setLeft(std::unique_ptr<ASTNode> leftNode) { left = std::move(leftNode); }
        void setRight(std::unique_ptr<ASTNode> rightNode) { right = std::move(rightNode); }
        void setOperator(TokenType op) { operator_ = op; }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitBinaryOpNode(this);
        }
    };
}
