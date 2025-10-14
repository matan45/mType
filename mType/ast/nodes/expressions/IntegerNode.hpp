#pragma once
#include "../../ASTNode.hpp"

namespace ast::nodes::expressions
{
    class IntegerNode : public ASTNode
    {
    private:
        int value;

    public:
        explicit IntegerNode(int val, const SourceLocation& loc = SourceLocation());

        int getValue() const;
        void setValue(int val);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
