#pragma once
#include "../../ASTNode.hpp"

namespace ast::nodes::expressions
{
    class NullNode : public ASTNode
    {
    public:
        explicit NullNode(const SourceLocation& loc = SourceLocation())
            : ASTNode(loc) {}

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitNullNode(this);
        }
    };
}
