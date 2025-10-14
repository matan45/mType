#pragma once
#include "../../ASTNode.hpp"
#include <memory>

namespace ast::nodes::statements
{
    class ForNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> initialization;
        std::unique_ptr<ASTNode> condition;
        std::unique_ptr<ASTNode> update;
        std::unique_ptr<ASTNode> body;

    public:
        explicit ForNode(std::unique_ptr<ASTNode> init, std::unique_ptr<ASTNode> cond, 
                std::unique_ptr<ASTNode> upd, std::unique_ptr<ASTNode> bodyStmt,
                const SourceLocation& loc = SourceLocation());

        ASTNode* getInitialization() const;
        ASTNode* getCondition() const;
        ASTNode* getUpdate() const;
        ASTNode* getBody() const;

        void setInitialization(std::unique_ptr<ASTNode> init);
        void setCondition(std::unique_ptr<ASTNode> cond);
        void setUpdate(std::unique_ptr<ASTNode> upd);
        void setBody(std::unique_ptr<ASTNode> bodyStmt);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
