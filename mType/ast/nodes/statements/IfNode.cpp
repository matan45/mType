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

    std::unique_ptr<ASTNode> IfNode::clone() const
    {
        std::unique_ptr<ASTNode> clonedCondition = condition ? condition->clone() : nullptr;
        std::unique_ptr<ASTNode> clonedThen = thenStatement ? thenStatement->clone() : nullptr;
        std::unique_ptr<ASTNode> clonedElse = elseStatement ? elseStatement->clone() : nullptr;

        return std::make_unique<IfNode>(std::move(clonedCondition), std::move(clonedThen),
                                        std::move(clonedElse), location);
    }
}
