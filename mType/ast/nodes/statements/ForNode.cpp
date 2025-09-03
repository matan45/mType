#include "ForNode.hpp"

namespace ast::nodes::statements
{
    ForNode::ForNode(std::unique_ptr<ASTNode> init, std::unique_ptr<ASTNode> cond, std::unique_ptr<ASTNode> upd,
                     std::unique_ptr<ASTNode> bodyStmt, const SourceLocation& loc)
        : ASTNode(loc), initialization(std::move(init)), condition(std::move(cond)),
          update(std::move(upd)), body(std::move(bodyStmt))
    {
    }

    ASTNode* ForNode::getInitialization() const
    {
        return initialization.get();
    }

    ASTNode* ForNode::getCondition() const
    {
        return condition.get();
    }

    ASTNode* ForNode::getUpdate() const
    {
        return update.get();
    }

    ASTNode* ForNode::getBody() const
    {
        return body.get();
    }

    void ForNode::setInitialization(std::unique_ptr<ASTNode> init)
    {
        initialization = std::move(init);
    }

    void ForNode::setCondition(std::unique_ptr<ASTNode> cond)
    {
        condition = std::move(cond);
    }

    void ForNode::setUpdate(std::unique_ptr<ASTNode> upd)
    {
        update = std::move(upd);
    }

    void ForNode::setBody(std::unique_ptr<ASTNode> bodyStmt)
    {
        body = std::move(bodyStmt);
    }

    Value ForNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitForNode(this);
    }
}
