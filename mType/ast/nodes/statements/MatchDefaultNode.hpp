#pragma once
#include "../../ASTNode.hpp"
#include <memory>

namespace ast::nodes::statements
{
    class MatchDefaultNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> body;

    public:
        explicit MatchDefaultNode(std::unique_ptr<ASTNode> body,
                                  const SourceLocation& loc = SourceLocation());

        ASTNode* getBody() const;
        void setBody(std::unique_ptr<ASTNode> newBody);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
