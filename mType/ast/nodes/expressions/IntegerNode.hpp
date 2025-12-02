#pragma once
#include "../../ASTNode.hpp"

namespace ast::nodes::expressions
{
    class IntegerNode : public ASTNode
    {
    private:
        int64_t value;

    public:
        explicit IntegerNode(int64_t val, const SourceLocation& loc = SourceLocation());

        int64_t getValue() const;
        void setValue(int64_t val);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
