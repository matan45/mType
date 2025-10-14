#pragma once
#include "../../ASTNode.hpp"
#include <memory>

namespace ast::nodes::statements
{
    class WhileNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> condition;
        std::unique_ptr<ASTNode> body;

    public:
        explicit WhileNode(std::unique_ptr<ASTNode> cond, std::unique_ptr<ASTNode> bodyStmt, const SourceLocation& loc = SourceLocation());

        ASTNode* getCondition() const;
        ASTNode* getBody() const;

        void setCondition(std::unique_ptr<ASTNode> cond);
        void setBody(std::unique_ptr<ASTNode> bodyStmt);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
