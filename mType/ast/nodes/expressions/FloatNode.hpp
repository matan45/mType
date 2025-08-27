#pragma once
#include "../../ASTNode.hpp"

namespace ast::nodes::expressions
{
    class FloatNode : public ASTNode
    {
    private:
        float value;

    public:
        explicit FloatNode(float val, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), value(val) {}

        float getValue() const { return value; }
        void setValue(float val) { value = val; }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitFloatNode(this);
        }
    };
}
