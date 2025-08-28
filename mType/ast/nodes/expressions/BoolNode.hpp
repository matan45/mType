#pragma once
#include "../../ASTNode.hpp"

namespace ast::nodes::expressions
{
    class BoolNode : public ASTNode
    {
    private:
        bool value;

    public:
        explicit BoolNode(bool val, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), value(val) {}

        bool getValue() const { return value; }
        void setValue(bool val) { value = val; }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitBoolNode(this);
        }
    };
}
