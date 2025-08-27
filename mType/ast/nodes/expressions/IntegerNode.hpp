#pragma once
#include "../../ASTNode.hpp"

namespace ast::nodes::expressions
{
    class IntegerNode : public ASTNode
    {
    private:
        int value;

    public:
        explicit IntegerNode(int val, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), value(val) {}

        int getValue() const { return value; }
        void setValue(int val) { value = val; }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitNumberNode(this);
        }
    };
}
