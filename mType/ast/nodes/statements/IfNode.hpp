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
               std::unique_ptr<ASTNode> elseStmt = nullptr, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), condition(std::move(cond)), thenStatement(std::move(thenStmt)), elseStatement(std::move(elseStmt)) {}

        std::unique_ptr<ASTNode> getCondition() const { return condition.get(); }
        std::unique_ptr<ASTNode> getThenStatement() const { return thenStatement.get(); }
        std::unique_ptr<ASTNode> getElseStatement() const { return elseStatement.get(); }

        void setCondition(std::unique_ptr<ASTNode> cond) { condition = std::move(cond); }
        void setThenStatement(std::unique_ptr<ASTNode> thenStmt) { thenStatement = std::move(thenStmt); }
        void setElseStatement(std::unique_ptr<ASTNode> elseStmt) { elseStatement = std::move(elseStmt); }

        bool hasElseStatement() const { return elseStatement != nullptr; }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitIfNode(this);
        }
    };
}
