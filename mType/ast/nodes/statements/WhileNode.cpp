#include "WhileNode.hpp"

namespace ast::nodes::statements
{
    WhileNode::WhileNode(std::unique_ptr<ASTNode> cond, std::unique_ptr<ASTNode> bodyStmt, const SourceLocation& loc)
        : ASTNode(loc), condition(std::move(cond)), body(std::move(bodyStmt))
    {
    }

    ASTNode* WhileNode::getCondition() const
    {
        return condition.get();
    }

    ASTNode* WhileNode::getBody() const
    {
        return body.get();
    }

    void WhileNode::setCondition(std::unique_ptr<ASTNode> cond)
    {
        condition = std::move(cond);
    }

    void WhileNode::setBody(std::unique_ptr<ASTNode> bodyStmt)
    {
        body = std::move(bodyStmt);
    }

    Value WhileNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitWhileNode(this);
    }
}
