#pragma once
#include "../../ASTNode.hpp"

namespace ast::nodes::statements
{
    class ContinueNode : public ASTNode
    {
    public:
        explicit ContinueNode(const SourceLocation& loc = SourceLocation())
            : ASTNode(loc) {}

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitContinueNode(this);
        }
    };
}
