#include "IfNode.hpp"

namespace ast::nodes::statements
{
    IfNode::IfNode(std::unique_ptr<ASTNode> cond, std::unique_ptr<ASTNode> thenStmt, std::unique_ptr<ASTNode> elseStmt,
                   const SourceLocation& loc)
        : ASTNode(loc), condition(std::move(cond)), thenStatement(std::move(thenStmt)),
          elseStatement(std::move(elseStmt))
    {
    }

    ASTNode* IfNode::getCondition() const
    {
        return condition.get();
    }

    ASTNode* IfNode::getThenStatement() const
    {
        return thenStatement.get();
    }

    ASTNode* IfNode::getElseStatement() const
    {
        return elseStatement.get();
    }

    void IfNode::setCondition(std::unique_ptr<ASTNode> cond)
    {
        condition = std::move(cond);
    }

    void IfNode::setThenStatement(std::unique_ptr<ASTNode> thenStmt)
    {
        thenStatement = std::move(thenStmt);
    }

    void IfNode::setElseStatement(std::unique_ptr<ASTNode> elseStmt)
    {
        elseStatement = std::move(elseStmt);
    }

    bool IfNode::hasElseStatement() const
    {
        return elseStatement != nullptr;
    }

    Value IfNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitIfNode(this);
    }
}
