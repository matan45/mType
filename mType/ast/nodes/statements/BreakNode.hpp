#pragma once
#include "../../ASTNode.hpp"

namespace ast::nodes::statements
{
    class BreakNode : public ASTNode
    {
    public:
        explicit BreakNode(const SourceLocation& loc = SourceLocation())
            : ASTNode(loc) {}

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitBreakNode(this);
        }
    };
}
