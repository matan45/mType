#pragma once
#include "../../ASTNode.hpp"

namespace ast::nodes::expressions
{
    class FloatNode : public ASTNode
    {
    private:
        float value;

    public:
        explicit FloatNode(float val, const SourceLocation& loc = SourceLocation());

        float getValue() const;
        void setValue(float val);

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
