#pragma once
#include "../../ASTNode.hpp"
#include <memory>

namespace ast::nodes::functions
{
    class ReturnNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> returnValue;

    public:
        explicit ReturnNode(std::unique_ptr<ASTNode> value = nullptr, const SourceLocation& loc = SourceLocation());

        std::unique_ptr<ASTNode> getReturnValue() const;
        void setReturnValue(std::unique_ptr<ASTNode> value);

        bool hasReturnValue() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
