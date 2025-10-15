#pragma once
#include "../../ASTNode.hpp"

namespace ast::nodes::statements
{
    class ContinueNode : public ASTNode
    {
    public:
        explicit ContinueNode(const SourceLocation& loc = SourceLocation());

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
