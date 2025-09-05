#pragma once
#include "../../ASTNode.hpp"

namespace ast::nodes::expressions
{
    class BoolNode : public ASTNode
    {
    private:
        bool value;

    public:
        explicit BoolNode(bool val, const SourceLocation& loc = SourceLocation());

        bool getValue() const;
        void setValue(bool val);

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
