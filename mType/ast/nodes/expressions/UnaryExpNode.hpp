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
                             const SourceLocation& loc = SourceLocation());

        TokenType getOperator() const;
        ASTNode* getOperand() const;
        UnaryPosition getPosition() const;

        bool isPrefix() const;
        bool isPostfix() const;

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}