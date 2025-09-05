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
                      const SourceLocation& loc = SourceLocation());

        ASTNode* getLeft() const;
        ASTNode* getRight() const;
        TokenType getOperator() const;

        void setLeft(std::unique_ptr<ASTNode> leftNode);
        void setRight(std::unique_ptr<ASTNode> rightNode);
        void setOperator(TokenType op);

        Value accept(ASTVisitor<Value>& visitor) override; 
    };
}
