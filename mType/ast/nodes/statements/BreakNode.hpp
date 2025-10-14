#pragma once
#include "../../ASTNode.hpp"

namespace ast::nodes::statements
{
    class BreakNode : public ASTNode
    {
    public:
        explicit BreakNode(const SourceLocation& loc = SourceLocation());

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
