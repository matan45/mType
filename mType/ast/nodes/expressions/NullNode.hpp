#pragma once
#include "../../ASTNode.hpp"

namespace ast::nodes::expressions
{
    class NullNode : public ASTNode
    {
    public:
        explicit NullNode(const SourceLocation& loc = SourceLocation());

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
