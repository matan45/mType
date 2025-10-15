#pragma once
#include "../../ASTNode.hpp"
#include <memory>

namespace ast::nodes::statements
{
    class IfNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> condition;
        std::unique_ptr<ASTNode> thenStatement;
        std::unique_ptr<ASTNode> elseStatement;

    public:
        explicit IfNode(std::unique_ptr<ASTNode> cond, std::unique_ptr<ASTNode> thenStmt, 
               std::unique_ptr<ASTNode> elseStmt = nullptr, const SourceLocation& loc = SourceLocation());

        ASTNode* getCondition() const;
        ASTNode* getThenStatement() const;
        ASTNode* getElseStatement() const;

        void setCondition(std::unique_ptr<ASTNode> cond);
        void setThenStatement(std::unique_ptr<ASTNode> thenStmt);
        void setElseStatement(std::unique_ptr<ASTNode> elseStmt);

        bool hasElseStatement() const;

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
