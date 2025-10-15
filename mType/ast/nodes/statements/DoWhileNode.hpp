#pragma once
#include "../../ASTNode.hpp"
#include <memory>

namespace ast::nodes::statements
{
    class DoWhileNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> body;
        std::unique_ptr<ASTNode> condition;

    public:
        explicit DoWhileNode(std::unique_ptr<ASTNode> bodyStmt, std::unique_ptr<ASTNode> cond,
                             const SourceLocation& loc = SourceLocation());
       
        ASTNode* getBody() const;
        ASTNode* getCondition() const;

        void setBody(std::unique_ptr<ASTNode> bodyStmt);
        void setCondition(std::unique_ptr<ASTNode> cond);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
