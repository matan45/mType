#pragma once
#include "../../ASTNode.hpp"

namespace ast::nodes::expressions
{
    class FloatNode : public ASTNode
    {
    private:
        double value;

    public:
        explicit FloatNode(double val, const SourceLocation& loc = SourceLocation());

        double getValue() const;
        void setValue(double val);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
