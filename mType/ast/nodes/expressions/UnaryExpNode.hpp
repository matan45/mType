#pragma once
#include "../../ASTNode.hpp"
#include "../../../token/TokenType.hpp"
#include <memory>

namespace ast::nodes::expressions
{
    using namespace token;

    enum class UnaryPosition
    {
        PREFIX,   // ++x, --x, -x, +x, !x
        POSTFIX   // x++, x--
    };

    class UnaryExpNode : public ASTNode
    {
    private:
        TokenType operator_;
        std::unique_ptr<ASTNode> operand;
        UnaryPosition position;

    public:
        explicit UnaryExpNode(TokenType op, std::unique_ptr<ASTNode> operand_, UnaryPosition pos = UnaryPosition::PREFIX,
                             const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), operator_(op), operand(std::move(operand_)), position(pos) {}

        TokenType getOperator() const { return operator_; }
        std::unique_ptr<ASTNode> getOperand() const { return operand.get(); }
        UnaryPosition getPosition() const { return position; }

        void setOperand(std::unique_ptr<ASTNode> operand_) {
            operand = std::move(operand_);
        }

        void setPosition(UnaryPosition pos) {
            position = pos;
        }

        bool isPrefix() const { return position == UnaryPosition::PREFIX; }
        bool isPostfix() const { return position == UnaryPosition::POSTFIX; }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitUnaryOpNode(this);
        }
    };
}